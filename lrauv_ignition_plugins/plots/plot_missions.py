#!/usr/bin/env python3

# Plot data from missions
# Usage:
#   $ python3 plot_missions.py

import os
import csv
import sys

import matplotlib.pyplot as plt
import numpy as np


def read_input_list(filename):

  timestamps_file = open(filename)
  timestamps = timestamps_file.readlines()
  timestamps = [stamp.strip() for stamp in timestamps]

  return timestamps


# Returns a list of values
# filename: Name of csv file. First row is headers
def read_csv(filename, field):

  print('Reading %s from %s' % (field, filename))

  # Ordered list, to keep indices in tact for retrieving assignment results
  vals = []

  with open(filename, 'r') as csvfile:
    reader = csv.DictReader(csvfile)
    fieldnames = reader.fieldnames
    for row in reader:
      vals.append(row[field])

  return vals


# infile: Path to csv file
# xvals: List containing x values to plot
# yvarname: y-variable name, header field in csv colume
# ax: Matplotlib axis object
def read_and_plot_one_variable(infile, xvals, yvarname, ax, lbl, color=None):

  yvals = read_csv(infile, yvarname)

  # Eliminate the unit at the end, save just the number
  units = yvals[len(yvals)-1].split()[1]
  yvals = [float(val.split()[0]) for val in yvals]

  line, = ax.plot(xvals, yvals, color=color, alpha=0.5)
  line.set_label(lbl)

  #plt.xticks(rotation=25)
  ax.set_title(yvarname)
  ax.set_ylabel(yvarname + ' (' + units + ')')
  ax.set_xlabel('Time (s)')


def main():

  this_path = os.path.dirname(os.path.realpath(__file__))
  missions_path = os.path.join(this_path, 'missions')

  blue = '#4682B4'
  orange = '#FF8C00'

  missionName = None

  # Uncomment the mission you want to plot. Plot configurations like axes,
  # labels, and legends are different for each mission.

  '''
  # VBS
  missionName = 'testDepthVBS'
  var = [
    'depth',
    'VerticalControl.depthCmd',
    'platform_buoyancy_position',
    'VerticalControl.buoyancyAction',
  ]
  varaxs = [0, 0, 1, 1]
  # Input data
  timestamps = read_input_list(os.path.join(missions_path, missionName,
    'plot_input_ref.txt'))
  lbls = [
    'state', 'cmd', 
    'state', 'cmd', 
  ]
  colors = [orange, blue, orange, blue]
  nPlots = max(varaxs) + 1
  '''

  '''
  # Mass shifter
  missionName = 'testPitchMass'
  var = [
    'platform_mass_position',
    'VerticalControl.massPositionAction',
    'platform_pitch_angle',
  ]
  # Subplot to put each variable
  varaxs = [0, 0, 1]
  # Input data
  timestamps = read_input_list(os.path.join(missions_path, missionName,
    'plot_input_ref.txt'))
  # Legend label for axis [0]
  lbls = ['state', 'cmd', 'state']
  # Color for each variable
  colors = [orange, blue, orange]
  nPlots = max(varaxs) + 1
  '''

  '''
  # Mass shifter + VBS
  missionName = 'testPitchAndDepthMassVBS'
  var = [
    'depth',
    'VerticalControl.depthCmd',
    'platform_buoyancy_position',
    'VerticalControl.buoyancyAction',
    'platform_mass_position',
    'VerticalControl.massPositionAction',
    'platform_pitch_angle',
  ]
  varaxs = [0, 0, 1, 1, 2, 2, 3]
  # Input data
  timestamps = read_input_list(os.path.join(missions_path, missionName,
    'plot_input_ref.txt'))
  lbls = [
    'state', 'cmd',
    'state', 'cmd',
    'state', 'cmd',
    'state',
  ]
  colors = [orange, blue, orange, blue, orange, blue, orange]
  nPlots = max(varaxs) + 1
  '''

  #'''
  # Yoyo
  missionName = 'testYoYoCircle'
  var = [
    'depth',
    'VerticalControl.elevatorAngleAction',
    'platform_pitch_angle',
  ]
  # Subplot to put each variable
  varaxs = [0, 1, 2]
  colors = None
  nPlots = max(varaxs) + 1
  # Input data
  timestamps = read_input_list(os.path.join(missions_path, missionName,
    'plot_input_ref.txt'))
  # Legend label for axis [0]
  lbls = ['Run ' + str(i) for i in range(len(timestamps))]
  #'''

  # Sanity check for user error
  if missionName is None:
    print('ERROR: missionName is None. Did you forget to uncomment a mission?')
    return

  title_suffix = ''
  if len(timestamps) == 1:
    title_suffix = timestamps[0]
  else:
    title_suffix = timestamps[0][:8] + 'multiRuns'


  # Output directory for plot image
  out_path = missionName
  out_path = os.path.join(missions_path, out_path)
  if not os.path.exists(out_path):
    os.makedirs(out_path)


  # Arg is ratio of height to width
  w, h = plt.figaspect(nPlots * 0.73)
  fig, axs = plt.subplots(nPlots, 1, figsize=(w, h), dpi=300)

  # Space between subplots
  plt.subplots_adjust(hspace=1)

  # Plot each mission log file
  for t_i in range(len(timestamps)):

    infile = os.path.join(missions_path, missionName, timestamps[t_i] + '.csv')

    # x-axis is time
    secs = read_csv(infile, 'EpochSeconds')
    # Convert string to numbers
    secs = np.array([float(sec) for sec in secs])
    # Offset from first timestamp, so can have some reasonable xticks
    secs = secs - secs[0]

    lbl = ''
    # Legend per file
    if missionName == 'testYoYoCircle':
      lbl = lbls[t_i]

    # Plot each variable in the corresponding subplot
    for v_i in range(len(var)):
      # Legend by variable name
      if missionName != 'testYoYoCircle':
        lbl = lbls[v_i]

      color = None
      if colors != None:
        color = colors[v_i]
      read_and_plot_one_variable(infile, secs, var[v_i], axs[varaxs[v_i]], lbl,
        color)

  # Only need legend on one subplot, all the colors are the same order
  axs[0].legend()

  # Overall figure title
  fig.suptitle(missionName + '\n' + title_suffix + ', %d runs' % len(timestamps))

  fig.savefig(os.path.join(out_path, title_suffix + '_' + missionName + '.png'), bbox_inches='tight')
  plt.show()


if __name__ == '__main__':
  main()