#!/usr/bin/env python3

import matplotlib.pyplot as plt
import matplotlib.colors as colors
import numpy as np

with open('size.txt') as f:
    SIZE = int(f.read())

SIZE /= 1024.0*1024.0

means = np.loadtxt('mean.txt')
mins = np.loadtxt('min.txt')
var = np.loadtxt('var.txt')
nsamples = np.loadtxt('nsamples.txt')

DATA = [(means, 'means.png', 'mean'),
        (np.sqrt(var), 'conf.png', 'standard error')]

for data, fname, desc in DATA:
    print('{}: min={}, max={}'.format(fname, np.min(data), np.max(data)))

    #means[10:20, 10:20] = 100
    fix, ax = plt.subplots(1)
    plt.imshow(data, norm=colors.Normalize(np.min(data), np.percentile(data, 99)),
               extent=(0, SIZE, 0, SIZE), origin='lower')
    plt.colorbar()
    plt.title('Cycles between two inc byte [eax]; {} (n={:g})'.format(desc, np.sum(nsamples)))
    plt.ylabel('First access offset (MiB)')
    plt.xlabel('Second access offset (MiB)')
    plt.savefig(fname)
