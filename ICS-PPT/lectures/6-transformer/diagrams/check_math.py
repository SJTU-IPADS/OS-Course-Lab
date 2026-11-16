#!/usr/bin/env python3
"""Check the numerical examples and the identities used by the lecture.

Run with Python's standard library. This validates the examples, not a model
benchmark. Cache equivalence is checked independently using a two-layer toy
causal network with fixed projections, per-position normalization, and an FFN.
"""
import importlib.util
import math
import pathlib
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

def check_attention_example():
    # The hand-computed head of attention-example.py: recompute it column by
    # column and compare with the figure's own numbers and with the slide.
    path=pathlib.Path(__file__).with_name('attention-example.py')
    spec=importlib.util.spec_from_file_location('attention_example',path)
    fig=importlib.util.module_from_spec(spec);spec.loader.exec_module(fig)
    tokens=[[row[j] for row in fig.E] for j in range(2)]
    q=[matvec(fig.W_Q,e) for e in tokens];k=[matvec(fig.W_K,e) for e in tokens];v=[matvec(fig.W_V,e) for e in tokens]
    assert q==[[1,1,1,0],[0,2,2,2]] and k==[[0,1,1,1],[1,0,1,0]] and v==[[1,1,0,0],[0,1,1,1]],(q,k,v)
    scores=[[sum(a*b for a,b in zip(k[i],q[j])) for j in range(2)] for i in range(2)]
    assert scores==[[2,6],[2,2]],scores
    weights=[softmax([scores[i][j]/math.sqrt(4) for i in range(2)]) for j in range(2)]
    out=[[sum(weights[j][i]*v[i][r] for i in range(2)) for r in range(4)] for j in range(2)]
    shown=lambda xs:[f'{x:.2f}' for x in xs]
    assert shown(weights[0])==['0.50','0.50'] and shown(weights[1])==['0.88','0.12'],weights
    assert shown(out[0])==['0.50','1.00','0.50','0.50'] and shown(out[1])==['0.88','1.00','0.12','0.12'],out
    steps=fig.steps()
    for name,cols in (('Q',q),('K',k),('V',v),('A',weights),('out',out)):
        for j,col in enumerate(cols):close([row[j] for row in steps[name]],col)
    for i in range(2):close(steps['scores'][i],scores[i]);close(steps['scaled'][i],[x/2 for x in scores[i]])

def main():
    check_attention_example()
    error=check_cache()
    print(f'Math examples passed; two-layer KV/full-prefix max error: {error:.3g}')

if __name__=='__main__':main()
