#!/usr/bin/env bash 

git clone https://compilers.ispras.ru/git/qbe.git qbe-lib ; cd qbe-lib && make ; make install DESTDIR=..