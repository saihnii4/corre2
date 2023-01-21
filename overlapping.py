import numpy as np
import time

env = np.zeros((2, 16, 5, 8))

object = [
    [0, 1, 0],
    [1, 1, 1],
    [0, 1, 0]
]

(y, x) = (len(object), len(object[0]))

start = env[0][0]

for i, row in enumerate(start[:y]):
    row[:x] = object[i]

print(start)

def move_right(start):
    right = np.insert(np.transpose(start), 0, np.zeros(5), axis=0)
    return np.transpose(right[:8])

while True:
    print(start)
    start = move_right(start)
    time.sleep(1)
