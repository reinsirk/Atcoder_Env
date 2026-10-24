# SPDX-License-Identifier: CC0-1.0
from pathlib import Path
from collections import deque
import json,random,hashlib
import argparse
parser=argparse.ArgumentParser(description='Project Selectionの入力を独立な全割当・増加路参照で生成する。')
parser.add_argument('--output-dir',type=Path,default=Path('/tmp/sirkit_project_selection_cases'))
p=parser.parse_args().output_dir
p.mkdir(parents=True,exist_ok=True)
d=p/'cases';d.mkdir(exist_ok=True);rng=random.Random(20261022)
def brute(n,w,a,keys):
 answer=0
 for mask in range(1<<n):
  if any((mask>>j)&1 and not (mask>>i)&1 for i,js in enumerate(keys) for j in js):continue
  answer=max(answer,sum(a[i]-w for i in range(n)if(mask>>i)&1))
 return answer
def reference(n,w,a,keys):
 source=n;sink=n+1;nodes=n+2;cap=[[0]*nodes for _ in range(nodes)];positive=0;bound=1+sum(abs(x-w)for x in a)
 for i,value in enumerate(a):
  weight=value-w
  if weight>0:cap[source][i]=weight;positive+=weight
  else:cap[i][sink]=-weight
 for i,js in enumerate(keys):
  for j in js:cap[j][i]+=bound
 flow=0
 while True:
  parent=[-1]*nodes;parent[source]=source;queue=deque([source])
  while queue and parent[sink]<0:
   v=queue.popleft()
   for u in range(nodes):
    if cap[v][u] and parent[u]<0:parent[u]=v;queue.append(u)
  if parent[sink]<0:break
  amount=bound;u=sink
  while u!=source:amount=min(amount,cap[parent[u]][u]);u=parent[u]
  u=sink
  while u!=source:v=parent[u];cap[v][u]-=amount;cap[u][v]+=amount;u=v
  flow+=amount
 return positive-flow
rows=[]
def save(name,n,w,a,keys,official=False,expected=None):
 text=f'{n} {w}\n'+' '.join(map(str,a))+'\n'+''.join(str(len(js))+(' '+' '.join(str(j+1)for j in js)if js else '')+'\n'for js in keys);value=reference(n,w,a,keys)
 if n<=12:assert value==brute(n,w,a,keys)
 if expected is not None:assert value==expected
 inp=d/(name+'.in');out=d/(name+'.out');inp.write_text(text);out.write_text(str(value)+'\n');rows.append({'name':name,'input':str(inp),'expected':str(out),'n':n,'constraints':sum(map(len,keys)),'official_sample':official,'input_sha256':hashlib.sha256(inp.read_bytes()).hexdigest(),'expected_sha256':hashlib.sha256(out.read_bytes()).hexdigest()})
save('sample_00',5,5,[5,2,10,3,6],[[2],[2],[],[4],[]],True,2);save('sample_01',6,10,[8,6,9,1,2,0],[[2],[2,3],[4],[4],[5],[]],True,0)
for trial in range(200):
 n=rng.randint(2,12);w=rng.randint(0,20);a=[rng.randint(0,20)for _ in range(n)];keys=[[j for j in range(i+1,n)if rng.randrange(4)==0]for i in range(n)];save(f'small_{trial:03}',n,w,a,keys)
for mode in ['dense','sparse','chain','star','none','positive','negative','zero','alternating','blocks']:
 n=100;w=5000000;a=[rng.randint(0,10000000)for _ in range(n)];keys=[[]for _ in range(n)]
 for i in range(n):
  for j in range(i+1,n):
   add=mode in ['dense','positive','negative','zero','alternating'] or mode=='sparse'and rng.randrange(15)==0 or mode=='chain'and j==i+1 or mode=='star'and i==0 or mode=='blocks'and i//10==j//10
   if add:keys[i].append(j)
 if mode=='positive':w=0;a=[10000000]*n
 if mode=='negative':w=10000000;a=[0]*n
 if mode=='zero':w=0;a=[0]*n
 if mode=='alternating':a=[10000000 if i%2 else 0 for i in range(n)]
 save('maximum_'+mode,n,w,a,keys)
(p/'cases.json').write_text(json.dumps(rows,indent=2)+'\n');print(len(rows),'project selection cases cross-checked; 200+2 small cases agree with exhaustive assignments')
