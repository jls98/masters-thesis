import sys
import math


def factorize(N, e, tri_dp, tri_dq):
    # patch tri-state to correct length
    max_factor_size = math.ceil(math.log2(N)) // 2
    assert max_factor_size == 2048
    p_factor_size = math.ceil(len(tri_dp))
    q_factor_size = math.ceil(len(tri_dq))

    # since we do not know the exact bit length, we guess / try all 3 possibilities that can ensure |N| = |p*q|
    increments = [(max_factor_size - p_factor_size - i, max_factor_size - q_factor_size - j) for i, j in [(0, 0), (1, 0), (0, 1)]]
    triv = []
    for inc_p, inc_q in increments:
        tri_dp_ = ([(0, 1)] * inc_p) + tri_dp + ([(0,)] * (max_factor_size - len(tri_dp) - inc_p))
        tri_dq_ = ([(0, 1)] * inc_q) + tri_dq + ([(0,)] * (max_factor_size - len(tri_dq) - inc_q))
        assert len(tri_dp_) == max_factor_size
        assert len(tri_dq_) == max_factor_size
        triv.append((tri_dp_, tri_dq_))

    factor_size = 1 << math.ceil(math.log2(len(tri_dp)))
    print("progress: ", end=" 0%", file=sys.stderr)
    for k in range(e):
        kp = ((e // 2) + pow(-1, k & 1) * (k>>1)) % e  # the most likely values for kp lie next to e/2
        if ((kp * (N - 1) + 1) % e) == 0:
            continue
        kq = (pow((kp * (N - 1) + 1), -1, e) * (1 - kp)) % e
        assert ((kp - 1) * (kq - 1) % e) == (kp * kq * N % e)

        print("\b\b\b" + ("%2d%%" % (100 * k / e)), end="", flush=True, file=sys.stderr)

        for tri_dp_, tri_dq_ in triv:
            opts = [(1, 1, 1, 1)]
            for i, (tri_dpv, tri_dqv) in enumerate(zip(tri_dp_[1:], tri_dq_[1:])):
                new_opts = []
                for p_, q_, dp_, dq_ in opts:
                    for pv, qv in [(0, 0), (0, 1), (1, 0), (1, 1)]:
                        p = p_ ^ (pv << (i + 1))
                        q = q_ ^ (qv << (i + 1))
                        if equiv(p * q, N, 1 << (i+2)):
                            for dpv in tri_dpv:
                                dp = dp_ ^ (dpv << (i + 1))
                                if equiv(e * dp, kp * (p - 1) + 1, 1 << (i+2)):
                                    for dqv in tri_dqv:
                                        dq = dq_ ^ (dqv << (i + 1))
                                        if equiv(e * dq, kq * (q - 1) + 1, 1 << (i+2)):
                                            new_opts.append((p, q, dp, dq))
                opts = new_opts
                if not opts:
                    break
            if opts:
                factors = [(p, q) for p, q, dp, dq in opts if p * q == N]
                if not factors:
                    print(" (failed)", file=sys.stderr)
                    return (None, None)
                print(" (completed)", file=sys.stderr)
                return factors[0]
    print("\b\b\b100%", file=sys.stderr)
    return (None, None)


def equiv(a, b, mod):
    return (a % mod) == (b % mod)

