#!/bin/env python3
import sys
from pathlib import Path
from typing import Any, Optional

from .parser import *
from .factorizer import factorize


def main(pname: str, trace: Optional[str] = None, Ns: Optional[str] = None, es: Optional[str] = "65537", *args: Any) -> Optional[int]:
    if None in [trace, Ns, es] or len(args) != 0:
        print("usage: %s <trace> <N> [e]" % pname, file=sys.stderr)
        return 1
    if len(set(Ns) - set(map(str, range(10)))) > 0:
        print("%s: N must be a number, not '%s'" % (pname, Ns), file=sys.stderr)
        return 1
    N = int(Ns)
    if len(set(es) - set(map(str, range(10)))) > 0:
        print("%s: e must be a number, not '%s'" % (pname, es), file=sys.stderr)
        return 1
    e = int(es)

    # load trace(s) as Square-and-Multiply sequence
    trace_path = Path(trace)
    if trace_path.is_file():
        seq_p, seq_q = load_trace(trace_path)
    else:
        # if a directory is given, load all traces and merge them to one
        seq_ps, seq_qs = [], []
        for trace_file in trace_path.glob("*"):
            if trace_file.is_file():
                seqs = load_trace(str(trace_file))
                if seqs is None:
                    continue
                seq_p, seq_q = seqs
                seq_ps.append(seq_p)
                seq_qs.append(seq_q)
        seq_p = merge_sequences(seq_ps)
        seq_q = merge_sequences(seq_qs)
    seqs = seq_p, seq_q
    print("Sequences:", file=sys.stderr)
    print("dp:", seq_p, file=sys.stderr)
    print("dq:", seq_q, file=sys.stderr)
    print("", file=sys.stderr)

    # convert to tri-state (0, 1, unknown)
    tri_p, tri_q = tris = [sam_sequence_to_tri_state(seq) for seq in seqs]
    print("Tri-States:", file=sys.stderr)
    print("dp = 0b%s (%d bit)" % (tri_state_to_string(tri_p), len(tri_state_to_string(tri_p))), file=sys.stderr)
    print("dq = 0b%s (%d bit)" % (tri_state_to_string(tri_q), len(tri_state_to_string(tri_q))), file=sys.stderr)
    print("", file=sys.stderr)

    p, q = factorize(N, e, tri_p, tri_q)
    if p is None or q is None:
        print("%s: factorization failed" % pname, file=sys.stderr)
        return 2
    print("factorization successful", file=sys.stderr)

    assert p * q == N
    print("p = %#x" % p)
    print("q = %#x" % q)
    d = pow(e, -1, (p-1)*(q-1))
    assert d * e % ((p-1)*(q-1)) == 1
    print("d = %#x" % d)

def factorgpg() -> int:
    return main(*sys.argv) or 0

