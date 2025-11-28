import matplotlib.pyplot as plt
import pandas as pd
from mpl_toolkits.mplot3d import Axes3D

df = pd.read_csv('debug_stars.csv')
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

# Plot stars
# Use magnitude for size (invert so brighter stars are bigger)
sizes = (6.0 - df['Mag']) * 2 
sizes[sizes < 0.1] = 0.1

ax.scatter(df['X'], df['Y'], df['Z'], s=sizes, c='white', edgecolors='black', linewidth=0.1)

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
plt.title('Star Catalog Verification')
plt.show()