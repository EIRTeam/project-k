# Python code
import numpy as np
import matplotlib.pyplot as plt

def plot_triangles_from_csv(points_file, roads_file):
  """Plots triangles defined by indices in a CSV file.

  Args:
    points_file: Path to the CSV file containing points (x, y).
    triangles_file: Path to the CSV file containing triangle indices.
  """

  # Load points
  points = np.loadtxt(points_file, delimiter=',')

  # Load triangle indices
  roads = np.loadtxt(roads_file, dtype=int, delimiter=',')

  # Extract x and y coordinates
  x, y, s = points[:, 0], points[:, 1], points[:, 2]
  colors = np.array(["#00FF00" if xi > 0 else "#0000FF" for xi in s])
  s = np.clip(s, 0, None)
  s *= 20.0

  fig, ax = plt.subplots()

  for road in roads:
    x_tri = [points[i][0] for i in road]
    y_tri = [points[i][1] for i in road]
    ax.plot(x_tri, y_tri, 'r-')  # Plot triangles in red

  ax.scatter(x, y, s=s, c=colors, zorder=100)  # Plot points

  plt.show()

points_file = 'vertices.csv'
edges_file = 'edges.csv'
plot_triangles_from_csv(points_file, edges_file)