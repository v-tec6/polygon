import numpy as np
data = list(map(int,input().split()))
arr = np.array(data)
def count_repeats(array):
    count = np.bincount(array) #создаем массив, с кол-вом повторений каждого числа
    res = count[array] #создаем массив, где проходимся по каждому элементу array
    return res
print(count_repeats(arr))

