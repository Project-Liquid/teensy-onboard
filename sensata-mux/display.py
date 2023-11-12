import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def calc_pressure(val):
    return 1450 * (val + 16000) / 32000

csv = pd.read_csv("log.txt")
data = csv.to_numpy()

sens0 = data[data[:, 0] == 0][:, 1]
sens1 = data[data[:, 0] == 1][:, 1]

fig, ax = plt.subplots()

ax.plot(calc_pressure(sens1), label="sensor 0")
ax.plot(calc_pressure(sens0), label="sensor 1")

ax.set_xlabel("index")
ax.set_ylabel("pressure (psi)")
ax.legend()

plt.show()
