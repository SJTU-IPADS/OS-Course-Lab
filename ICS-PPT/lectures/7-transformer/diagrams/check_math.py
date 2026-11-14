#!/usr/bin/env python3
"""Check the numerical examples and the identities used by the lecture.

Run with Python's standard library. This validates the examples, not a model
benchmark. Cache equivalence is checked independently using a two-layer toy
causal network with fixed projections, per-position normalization, and an FFN.
"""
import math
import random
from math_data import matvec, softmax

def close(actual,expected):
    assert len(actual)==len(expected)
    assert all(math.isclose(a,b,rel_tol=1e-11,abs_tol=1e-11) for a,b in zip(actual,expected)),(actual,expected)

def normalize(vector):
    mean=sum(vector)/len(vector)
    variance=sum((x-mean)**2 for x in vector)/len(vector)
    return [(x-mean)/math.sqrt(variance+1e-5) for x in vector]

def attention(query,keys,values):
    weights=softmax([sum(q*k for q,k in zip(query,key))/math.sqrt(len(query)) for key in keys])
    return [sum(w*value[col] for w,value in zip(weights,values)) for col in range(len(values[0]))]

def projected_state(vector,keys,values,matrices):
    wq,wk,wv,wo,w1,w2=matrices
    hidden=normalize(vector)
    query,key,value=[matvec(weight,hidden) for weight in (wq,wk,wv)]
    keys.append(key);values.append(value)
    update=matvec(wo,attention(query,keys,values))
    residual=[x+delta for x,delta in zip(vector,update)]
    ff=matvec(w2,[math.tanh(x) for x in matvec(w1,normalize(residual))])
    return [x+delta for x,delta in zip(residual,ff)]

def check_cache():
    rng=random.Random(7)
    def matrix(rows,cols):return [[rng.uniform(-.4,.4) for _ in range(cols)] for _ in range(rows)]
    layers=[(matrix(2,4),matrix(2,4),matrix(2,4),matrix(4,2),matrix(8,4),matrix(4,8)) for _ in range(2)]
    inputs=[[rng.uniform(-1,1)+.02*pos for _ in range(4)] for pos in range(7)]
    cache=[([],[]) for _ in layers]
    worst=0.0
    for stop in range(1,len(inputs)+1):
        # Reference: start from the whole prefix and recompute all layers.
        full=inputs[:stop]
        for layer in layers:
            keys,values=[],[]
            full=[projected_state(vector,keys,values,layer) for vector in full]
        # Cached path: only the new position traverses each layer.
        current=inputs[stop-1]
        for layer,(keys,values) in zip(layers,cache):
            current=projected_state(current,keys,values,layer)
        close(current,full[-1])
        worst=max(worst,max(abs(a-b) for a,b in zip(current,full[-1])))
    return worst

def main():
    # Horizontal output-matrix blocks times vertical head stacking.
    blocks=[[[1,2],[3,4]],[[2,-1],[0,1]],[[1,0],[-2,1]]]
    heads=[[1,2],[3,4],[5,6]]
    combined=[sum((block[row] for block in blocks),[]) for row in range(2)]
    separate=[sum(matvec(block,head)[row] for block,head in zip(blocks,heads)) for row in range(2)]
    close(matvec(combined,sum(heads,[])),separate)
    error=check_cache()
    print(f'Math examples passed; two-layer KV/full-prefix max error: {error:.3g}')

if __name__=='__main__':main()
