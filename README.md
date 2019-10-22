# OpenJij : Framework for the Ising model and QUBO.

[![Build Status](https://travis-ci.org/OpenJij/OpenJij.svg?branch=master)](https://travis-ci.org/OpenJij/OpenJij)

cmake >= 3.11

- [Tutorials](https://openjij.github.io/OpenJijTutorial/_build/html/index.html)
- [Documents](https://openjij.github.io/OpenJij/)

## install

### pip
```
$ pip install openjij
```

### cmake setup

If you had not installed cmake>=3.12, you need install cmake>=3.12

macOS
```
$ brew install cmake
```

Linux
```
# if you installed old version by apt-get
$ apt-get purge cmake

# install cmake 
$ wget https://cmake.org/files/v3.13/cmake-3.13.2.tar.gz
$ tar xvf cmake-3.13.2.tar.gz
$ cd cmake-3.13.2
$ ./bootstrap && make && sudo make install 
```

### Install openjij 
```
$ git clone git@github.com:OpenJij/OpenJij.git
$ cd openjij
$ python setup.py install
```

## How to use

### Python example

```python
import openjij as oj
sampler = oj.SASampler()
response = sampler.sample_ising(h={0: -1}, J={(0,1): -1})
response.states
# [[1,1]]

# with indices
response = sampler.sample_ising(h={'a': -1}, J={('a','b'): 1})
[{index: s for index, s in zip(response.indices, state)} for state in response.states]
# [{'b': -1, 'a': 1}]
```
## Community

- [OpenJij Slack](https://join.slack.com/t/openjij/shared_invite/enQtNjQyMjIwMzMwNzA4LWU4ODM5ODNjZmIyMzViOTZjODAyM2NmNmM3NTcwYmE2NGFkZGFkOGE3ZWVjMTNkOWZhOTVjMGE4NDI4YjlkMzc)

