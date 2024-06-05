from typing import Dict, List, Literal, Optional, Tuple, Union

import numpy


def load_trace(trace_file: str) -> Optional[Tuple[str, str]]:
    data = numpy.loadtxt(trace_file)
    if (seqs := trace_to_sam_sequences(data)) is not None:
        assert all("MM" not in seq for seq in seqs)
        seq_p, seq_q = seqs
        return (seq_p, seq_q)
    return None


def merge_sequences(seqs: List[str], merge_peak: bool = False) -> str:
    assert all("MM" not in seq and seq.startswith("S") and seq.endswith("S") and set(seq) == {"S", "M"} for seq in seqs)

    # split sequences into lists of consecuritve squarings
    seqs = [[len(sx) for sx in seq.split("M")] for seq in seqs]

    if merge_peak:
        # determine largest sequence length
        length = max(len(x) for x in seqs)
    else:
        # determine most common sequence length
        seq_lengths = {}
        for seq in seqs:
            if len(seq) not in seq_lengths:
                seq_lengths[len(seq)] = 0
            seq_lengths[len(seq)] += 1
        length = max(seq_lengths, key=lambda k: seq_lengths[k])

    # ignore sequences of different length
    fseqs = [seq for seq in seqs if len(seq) == length]
    assert merge_peak or (len(fseqs) / len(seqs) >= 1/2), (len(seqs), len(fseqs))

    # calculate average over all sequences of length 'length'
    final_seq = [round(sum(fseqs[j][i] for j in range(len(fseqs)))/len(fseqs)) for i in range(length)]

    # create string sequence from result
    return "M".join("S" * e for e in final_seq)


def tri_state_to_string(state: List[Union[Tuple[Literal[0]], Tuple[Literal[1]], Tuple[Literal[0], Literal[1]]]]) -> str:
    assert all(v in [(0,), (1,), (0, 1)] for v in state)
    return "".join("?" if i == (0, 1) else str(i[0]) for i in state[::-1])


def sam_sequence_to_tri_state(seq: str, w=2) -> List[Union[Tuple[Literal[0]], Tuple[Literal[1]], Tuple[Literal[0], Literal[1]]]]:
    assert set(seq) == {"S", "M"}
    assert seq.startswith("S")
    seq = seq.strip("M") + "M"
    assert "MM" not in seq

    size = seq.count("S") + 1
    res = [(1,)] + [(0, 1) for _ in range(size-1)]
    idx = 1
    while len(seq) > 0:
        assert seq[0] == "S"
        sqrs = seq.index("M")
        if (sqrs - w) > 0:
            for i in range(sqrs - w):
                res[idx + i] = (0,)
        res[idx + sqrs - 1] = (1,)

        idx += sqrs
        seq = seq[sqrs+1:]
    res[-1] = (0, 1)  # this bit might be wrongly detected
    return res[::-1]


def trace_to_peak_list(trace: List[int]) -> Dict[int, int]:
    result = {}
    last = None
    start = None
    length = 0
    lngthlst = []
    last = -100
    last = None
    for i, xi in enumerate(trace):
        if xi < 120:
            if start is None:
                start = last = i
                length = 1
            elif i - last < 4:
                # allow gaps of up to 3 misses
                last = i
                length += 1
            else:
                # ignore peaks, as they might be from speculative execution
                if length > 2:
                    result[start] = (length, i - start)
                start = None
                length = 0
    return result


def trace_to_sam_sequences(data):
    M = trace_to_peak_list(data[:, 0])
    S = trace_to_peak_list(data[:, 1])

    # join traces
    seq = []
    for i in range(len(data)):
        assert i not in S or i not in M
        if i in S:
            seq.append("S")
        elif i in M:
            seq.append("M")

    if (seq := "".join(seq)) == "":
        # abort if trace/sequence is empty
        return None

    # split into dp and dq sequences
    seq = seq.strip("M")
    assert len(seq.rsplit("MM")) >= 2, (len(seq.rsplit("MM")), seq)
    seq_p, seq_q = seq.rsplit("MM", 1)
    seq_p = seq_p.rstrip("M")

    if "MM" in seq_p or "MM" in seq_q:
        # abort if splitting was unsuccessful
        return None

    return seq_p, seq_q

