import numpy as np
import matplotlib.pyplot as plt

def avg_score(name):
    avgs = []
    with open(f'./log/{name}.log') as f:
        for s in f.readlines():
            if s.find('avg') != -1:
                # print(s)
                avgs.append(float(s.split()[3][:-1]))
    return avgs

x = np.arange(1000)
four_avgs = avg_score('four')
six_avgs = avg_score('six')
best_six_avgs = avg_score('best_six')

plt.plot(x, four_avgs, label='4-tuple')
plt.plot(x, six_avgs, label='6-tuple')
plt.plot(x, best_six_avgs, label='best 6 6-tuple')
plt.legend()
plt.show()