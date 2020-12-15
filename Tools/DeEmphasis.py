#based on https://github.com/gnuradio/gnuradio/blob/master/gr-analog/python/analog/fm_emph.py

import math
import cmath
import numpy as np
import scipy.signal as signal
import pylab as pl

tau = 750e-6
fs = 8000
fh = 2700

# Digital corner frequency
w_c = 1.0 / tau

# Prewarped analog corner frequency
w_ca = 2.0 * fs * math.tan(w_c / (2.0 * fs))

# Resulting digital pole, zero, and gain term from the bilinear
# transformation of H(s) = w_ca / (s + w_ca) to
# H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
k = -w_ca / (2.0 * fs)
z1 = -1.0
p1 = (1.0 + k) / (1.0 - k)
b0 = -k / (1.0 - k)

btaps = [ b0 * 1.0, b0 * -z1, 0 ]
ataps = [      1.0,      -p1, 0 ]

# Since H(s = 0) = 1.0, then H(z = 1) = 1.0 and has 0 dB gain at DC


taps = np.concatenate((btaps, ataps), axis=0)
print("Taps")
print(*taps, "", sep=",", end="\n")

f,h = signal.freqz(btaps,ataps, fs=fs)
pl.plot(f, 20*np.log10(np.abs(h)))
pl.xlabel('frequency/Hz')
pl.ylabel('gain/dB')
pl.ylim(top=0,bottom=-30)
pl.xlim(left=0, right=fh*2.5)
pl.show()