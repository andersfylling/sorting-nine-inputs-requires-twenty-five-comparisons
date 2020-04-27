import glob
import json
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


def read_json(files):
    results = []
    for file in files:
        with open(file) as json_file:
            results.append(json.load(json_file))
    return results


def read_durations(datas):
    results = []
    for data in datas:
        result = {
            'n': data['n'],
            'k': data['k'],
            'threads': data['cores'] + 1,
            'duration': data['duration'],
            'layers': []
        }
        for p in data['layers']:
            duration = {
                'generating': p['duration']['generating']['total'],
                'pruning': p['duration']['pruning']['total']
            }
            result['layers'].append(duration)
        results.append(result)
    return results


def plot(before, after):
    durations1 = []
    durations2 = []
    for i in range(0, len(before['layers'])):
        durations1.append(before['layers'][i]['generating'] + before['layers'][i]['pruning'])
        durations2.append(after['layers'][i]['generating'] + after['layers'][i]['pruning'])

    fig, ax = plt.subplots()
    ax.plot(durations1)
    ax.plot(durations2)

    ax.set(xlabel='layers', ylabel='durations (s)',
           title='N' + str(after['n']) + ', |K' + str(after['k']) + '|, with ' + str(after['threads']) + ' threads')
    ax.grid()

    fig.savefig("comparison-N" + str(after['n']) + ".png")
    plt.show()


files_cpp_metrics = glob.glob('results/metrics-cpp-*.json')
files_prolog_metrics = glob.glob('results/metrics-prolog-*.json')
#print(files_cpp_metrics)

jsons_cpp_metrics = read_json(files_cpp_metrics)
jsons_prolog_metrics = read_json(files_prolog_metrics)
#print(jsons_cpp_metrics)

durations_cpp_metrics = read_durations(jsons_cpp_metrics)
durations_prolog_metrics = read_durations(jsons_prolog_metrics)
#print(durations_cpp_metrics)

def sort_by_n(m):
    return m['n']

durations_prolog_metrics.sort(key=sort_by_n)
durations_cpp_metrics.sort(key=sort_by_n)

for i in range(0, len(durations_prolog_metrics)):
    plot(durations_prolog_metrics[i], durations_cpp_metrics[i])