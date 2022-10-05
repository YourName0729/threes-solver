import numpy as np
import matplotlib.pyplot as plt

arr = None
with open('anal.txt') as f:
    arr = np.array([float(x[:-1]) for x in f.readlines()])
    # print(f.readlines())
    # print(arr)
    # print(len(arr))
    # print(np.where(arr == 0.0))
    # print(arr.min(), arr.max())

width=5
# x = np.linspace(0, 100, 100 // width)

# print(len(x))
x = np.array([f'{5 * i}-{5 * (i + 1)}' for i in range(0, 100 // width)])
y = np.array([len(np.where(np.logical_and(arr >= width * i, arr < width * (i + 1)))[0]) for i in range(0, 100 // width)])

# print(len(y), y)
# print(x)
plt.pie(y[y != 0], labels= x[y != 0], autopct='%1.1f%%')
plt.show()