#!/usr/bin/python3

import os
import matplotlib.pyplot as plt
import numpy as np
import re

# collect data
os.system('cd ../parser/ && sh compile.sh')

os.system('../parser/a.out ../data/editlogs_10_complex.xml > 10-complex')
os.system('../parser/a.out ../data/editlogs_100_complex.xml > 100-complex')
os.system('../parser/a.out ../data/editlogs_1000_complex.xml > 1000-complex')
os.system('../parser/a.out ../data/editlogs_10000_complex.xml > 10000-complex')
os.system('../parser/a.out ../data/editlogs_100000_complex.xml > 100000-complex')
os.system('../parser/a.out ../data/editlogs_1000000_complex.xml > 1000000-complex')
os.system('../parser/a.out ../data/editlogs_10_plain.xml > 10-plain')
os.system('../parser/a.out ../data/editlogs_100_plain.xml > 100-plain')
os.system('../parser/a.out ../data/editlogs_1000_plain.xml > 1000-plain')
os.system('../parser/a.out ../data/editlogs_10000_plain.xml > 10000-plain')
os.system('../parser/a.out ../data/editlogs_100000_plain.xml > 100000-plain')
os.system('../parser/a.out ../data/editlogs_1000000_plain.xml > 1000000-plain')
os.system('../parser/a.out ../data/editlogs_10_moderate.xml > 10-moderate')
os.system('../parser/a.out ../data/editlogs_100_moderate.xml > 100-moderate')
os.system('../parser/a.out ../data/editlogs_1000_moderate.xml > 1000-moderate')
os.system('../parser/a.out ../data/editlogs_10000_moderate.xml > 10000-moderate')
os.system('../parser/a.out ../data/editlogs_100000_moderate.xml > 100000-moderate')
os.system('../parser/a.out ../data/editlogs_1000000_moderate.xml > 1000000-moderate')

compress_rate = dict()
serialize_time = dict()
deserialize_time = dict()

for i in os.listdir('./'):
    if '-' not in i:
        continue
    if not re.findall(r'10+', i.split('-')[0]):
        continue
    path = './' + i
    f = open(path,'r')
    for j in range(5):
        f.readline()
    s = float(f.readline().split()[3])
    d = float(f.readline().split()[3])
    c = float(f.readline().split()[3])
    data_size = int(i.split('-')[0])
    data_type = i.split('-')[1]
    if data_type not in serialize_time:
        serialize_time[data_type] = dict()
    serialize_time[data_type][data_size] = s
    if data_type not in deserialize_time:
        deserialize_time[data_type] = dict()
    deserialize_time[data_type][data_size] = d
    if data_type not in compress_rate:
        compress_rate[data_type] = dict()
    compress_rate[data_type][data_size] = c

#generate figures

fig, ax = plt.subplots()  # Create a figure containing a single axes.
ax.set_title('complex logs compression - compare')
ax.set_xscale('log')
ax.set_ylabel('compression (%)')
ax.set_xlabel('count of records')
x_ax=[10,100,1000,10000,100000,1000000]
y_ax_complex=[compress_rate['complex'][10],compress_rate['complex'][100],compress_rate['complex'][1000], compress_rate['complex'][10000],compress_rate['complex'][100000], compress_rate['complex'][1000000]]
y_ax_plain=[compress_rate['plain'][10], compress_rate['plain'][100],compress_rate['plain'][1000], compress_rate['plain'][10000],compress_rate['plain'][100000], compress_rate['plain'][1000000]]
y_ax_moderate=[compress_rate['moderate'][10], compress_rate['moderate'][100],compress_rate['moderate'][1000], compress_rate['moderate'][10000],compress_rate['moderate'][100000], compress_rate['moderate'][1000000]]
ax.plot(x_ax, y_ax_complex, label='complex')  # Plot some data on the axes.
ax.plot(x_ax, y_ax_moderate, label='moderate')  # Plot some data on the axes.
ax.plot(x_ax, y_ax_plain, label='plain')  # Plot some data on the axes.
ax.legend()
fig.savefig('compress=rate-compare.png')

fig, ax = plt.subplots()  # Create a figure containing a single axes.
ax.set_title('complex logs compression - average')
ax.set_xscale('log')
ax.set_ylabel('compression (%)')
ax.set_xlabel('count of records')
x_ax=[10,100,1000,10000,100000,1000000]
y_ax_complex=[compress_rate['complex'][10],compress_rate['complex'][100],compress_rate['complex'][1000], compress_rate['complex'][10000],compress_rate['complex'][100000], compress_rate['complex'][1000000]]
y_ax_plain=[compress_rate['plain'][10], compress_rate['plain'][100],compress_rate['plain'][1000], compress_rate['plain'][10000],compress_rate['plain'][100000], compress_rate['plain'][1000000]]
y_ax_moderate=[compress_rate['moderate'][10],compress_rate['moderate'][100],compress_rate['moderate'][1000], compress_rate['moderate'][10000],compress_rate['moderate'][100000], compress_rate['moderate'][1000000]]
y_ax=[(y_ax_moderate[0]+y_ax_complex[0]+y_ax_plain[0])/3,(y_ax_moderate[1]+y_ax_complex[1]+y_ax_plain[1])/3,(y_ax_moderate[2]+y_ax_complex[2]+y_ax_plain[2])/3,(y_ax_moderate[3]+y_ax_complex[3]+y_ax_plain[3])/3, (y_ax_moderate[4]+y_ax_complex[4]+y_ax_plain[4])/3, (y_ax_moderate[5]+y_ax_complex[5]+y_ax_plain[5])/3]
ax.plot(x_ax, y_ax, label='average')  # Plot some data on the axes.
ax.legend()
fig.savefig('compress=rate-average.png')

fig, ax = plt.subplots()  # Create a figure containing a single axes.
ax.set_title('serialization time - compare')
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_ylabel('time (us)')
ax.set_xlabel('count of records')
x_ax=[10,100,1000,10000,100000,1000000]
y_ax_complex=[serialize_time['complex'][10],serialize_time['complex'][100],serialize_time['complex'][1000], serialize_time['complex'][10000],serialize_time['complex'][100000], serialize_time['complex'][1000000]]
y_ax_plain=[serialize_time['plain'][10],serialize_time['plain'][100],serialize_time['plain'][1000], serialize_time['plain'][10000], serialize_time['plain'][100000], serialize_time['plain'][1000000]]
y_ax_moderate=[serialize_time['moderate'][10],serialize_time['moderate'][100],serialize_time['moderate'][1000], serialize_time['moderate'][10000],serialize_time['moderate'][100000], serialize_time['moderate'][1000000]]
ax.plot(x_ax, y_ax_complex, label='complex')  # Plot some data on the axes.
ax.plot(x_ax, y_ax_moderate, label='moderate')  # Plot some data on the axes.
ax.plot(x_ax, y_ax_plain, label='plain')  # Plot some data on the axes.
ax.legend()
fig.savefig('serialize-time-compare.png')

fig, ax = plt.subplots()  # Create a figure containing a single axes.
ax.set_title('serialization time - average')
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_ylabel('time (us)')
ax.set_xlabel('count of records')
x_ax=[10,100,1000,10000,100000,1000000]
y_ax_complex=[serialize_time['complex'][10],serialize_time['complex'][100],serialize_time['complex'][1000], serialize_time['complex'][10000],serialize_time['complex'][100000], serialize_time['complex'][1000000]]
y_ax_plain=[serialize_time['plain'][10],serialize_time['plain'][100],serialize_time['plain'][1000], serialize_time['plain'][10000],serialize_time['plain'][100000], serialize_time['plain'][1000000]]
y_ax_moderate=[serialize_time['moderate'][10],serialize_time['moderate'][100],serialize_time['moderate'][1000], serialize_time['moderate'][10000], serialize_time['moderate'][100000], serialize_time['moderate'][1000000]]
y_ax=[(y_ax_moderate[0]+y_ax_complex[0]+y_ax_plain[0])/3,(y_ax_moderate[1]+y_ax_complex[1]+y_ax_plain[1])/3,(y_ax_moderate[2]+y_ax_complex[2]+y_ax_plain[2])/3,(y_ax_moderate[3]+y_ax_complex[3]+y_ax_plain[3])/3, (y_ax_moderate[4]+y_ax_complex[4]+y_ax_plain[4])/3, (y_ax_moderate[5]+y_ax_complex[5]+y_ax_plain[5])/3]
y_ax_avg_se = y_ax
ax.plot(x_ax, y_ax, label='average')  # Plot some data on the axes.
ax.legend()
fig.savefig('serialize-time-average.png')


fig, ax = plt.subplots()  # Create a figure containing a single axes.
ax.set_title('deserialization time - compare')
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_ylabel('time (us)')
ax.set_xlabel('count of records')
x_ax=[10,100,1000,10000,100000,1000000]
y_ax_complex=[deserialize_time['complex'][10],deserialize_time['complex'][100],deserialize_time['complex'][1000], deserialize_time['complex'][10000],deserialize_time['complex'][100000], deserialize_time['complex'][1000000]]
y_ax_plain=[deserialize_time['plain'][10],deserialize_time['plain'][100],deserialize_time['plain'][1000], deserialize_time['plain'][10000],deserialize_time['plain'][100000], deserialize_time['plain'][1000000]]
y_ax_moderate=[deserialize_time['moderate'][10],deserialize_time['moderate'][100],deserialize_time['moderate'][1000], deserialize_time['moderate'][10000],deserialize_time['moderate'][100000], deserialize_time['moderate'][1000000]]
ax.plot(x_ax, y_ax_complex, label='complex')  # Plot some data on the axes.
ax.plot(x_ax, y_ax_moderate, label='moderate')  # Plot some data on the axes.
ax.plot(x_ax, y_ax_plain, label='plain')  # Plot some data on the axes.
ax.legend()
fig.savefig('deserialize-time-compare.png')

fig, ax = plt.subplots()  # Create a figure containing a single axes.
ax.set_title('deserialization time - average')
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_ylabel('time (us)')
ax.set_xlabel('count of records')
x_ax=[10,100,1000,10000,100000,1000000]
y_ax_complex=[deserialize_time['complex'][10],deserialize_time['complex'][100],deserialize_time['complex'][1000], deserialize_time['complex'][10000],deserialize_time['complex'][100000], deserialize_time['complex'][1000000]]
y_ax_plain=[deserialize_time['plain'][10],deserialize_time['plain'][100],deserialize_time['plain'][1000], deserialize_time['plain'][10000],deserialize_time['plain'][100000], deserialize_time['plain'][1000000]]
y_ax_moderate=[deserialize_time['moderate'][10],deserialize_time['moderate'][100],deserialize_time['moderate'][1000], deserialize_time['moderate'][10000],deserialize_time['moderate'][100000], deserialize_time['moderate'][1000000]]
y_ax=[(y_ax_moderate[0]+y_ax_complex[0]+y_ax_plain[0])/3,(y_ax_moderate[1]+y_ax_complex[1]+y_ax_plain[1])/3,(y_ax_moderate[2]+y_ax_complex[2]+y_ax_plain[2])/3,(y_ax_moderate[3]+y_ax_complex[3]+y_ax_plain[3])/3, (y_ax_moderate[4]+y_ax_complex[4]+y_ax_plain[4])/3, (y_ax_moderate[5]+y_ax_complex[5]+y_ax_plain[5])/3]
y_ax_avg_dese = y_ax
ax.plot(x_ax, y_ax, label='average')  # Plot some data on the axes.
ax.legend()
fig.savefig('deserialize-time-average.png')

fig, ax = plt.subplots()  # Create a figure containing a single axes.
ax.set_title('(de)serialization time - average compare')
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_ylabel('time (us)')
ax.set_xlabel('count of records')
x_ax=[10,100,1000,10000,100000,1000000]
ax.plot(x_ax, y_ax_avg_dese, label='deserialize')  # Plot some data on the axes.
ax.plot(x_ax, y_ax_avg_se, label='serialize')  # Plot some data on the axes.
ax.legend()
fig.savefig('(de)serialize-time-average-compare.png')