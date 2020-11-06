#based on https://github.com/gnuradio/gnuradio/blob/master/gr-analog/python/analog/fm_emph.py

import math
import cmath
import numpy as np
import scipy.signal as signal
import pylab as pl

tau = 750e-6
fs = 8000
fh = 2700

# Digital corner frequencies
w_cl = 1.0 / tau
w_ch = 2.0 * math.pi * fh

# Prewarped analog corner frequencies
w_cla = 2.0 * fs * math.tan(w_cl / (2.0 * fs))
w_cha = 2.0 * fs * math.tan(w_ch / (2.0 * fs))

# Resulting digital pole, zero, and gain term from the bilinear
# transformation of H(s) = (s + w_cla) / (s + w_cha) to
# H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
kl = -w_cla / (2.0 * fs)
kh = -w_cha / (2.0 * fs)
z1 = (1.0 + kl) / (1.0 - kl)
p1 = (1.0 + kh) / (1.0 - kh)
b0 = (1.0 - kl) / (1.0 - kh)

# Since H(s = infinity) = 1.0, then H(z = -1) = 1.0 and
# this filter  has 0 dB gain at fs/2.0.
# That isn't what users are going to expect, so adjust with a
# gain, g, so that H(z = 1) = 1.0 for 0 dB gain at DC.
w_0dB = 2.0 * math.pi * 0.0
g =        abs(1.0 - p1 * cmath.rect(1.0, -w_0dB)) \
/ (b0 * abs(1.0 - z1 * cmath.rect(1.0, -w_0dB)))

btaps = [ g * b0 * 1.0, g * b0 * -z1, 0]
ataps = [          1.0,          -p1, 0]

taps = np.concatenate((btaps, ataps), axis=0)
print("Taps")
print(*taps, "", sep=",", end="\n")

f,h = signal.freqz(btaps,ataps, fs=fs)
pl.plot(f, 20*np.log10(np.abs(h)))
pl.xlabel('frequency/Hz')
pl.ylabel('gain/dB')
pl.ylim(top=30,bottom=0)
pl.xlim(left=0, right=fh*2.5)
pl.show()