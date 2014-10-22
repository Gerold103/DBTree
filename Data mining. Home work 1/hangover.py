import numpy as np
import pylab as pl
import scipy.special

__author__ = 'vladislav shpilevoi'

def get_lists_of_wines(filename):
	"""Reads file filename and returns matrix with rows-concrete sorts of wine and cols-attributes of wine"""
	f = open(filename)
	strs = f.readlines()
	# matrix n X m
	n = len(strs)
	m = len(strs[0].split(','))
	wines = np.zeros((n, m), dtype=float)
	for i in range(n):
		curr_wine = strs[i].split(',')
		for j in range(m):
			wines[i, j] = float(curr_wine[j])
	return wines

def distance(wine1, wine2):
	"""Distance between wine1 and wine2"""
	wine11 = wine1.copy()
	wine22 = wine2.copy()
	wine11[0] = 0
	wine22[0] = 0
	return np.sqrt(((wine11 - wine22) * (wine11 - wine22)).sum())

def find_neighbour_indices_without(wines, wine, eps, n=-1):
	"""Returns list of indices of elements in wines which are closer to wine than eps"""
	result = []
	i = 0
	for w in wines:
		if ((distance(w, wine) <= eps) and (i != n)):
			result.append(i)
		i += 1
	return result

def merge_list2_to_list1(list1, list2):
	"""Merge list2 to list1. list1 will be changed"""
	for x in list2:
		if (x in list1):
			continue
		else:
			list1.append(x)

def dbscan(wines, eps, min_pts):
	"""Clusterization by DBSCAN"""
	C = 0
	# row num - index in wines, 0 - visited/not visited, 1 - cluster number
	sommelier = np.zeros((len(wines[:, 0]), 2), dtype=int)
	for i in range(len(sommelier[:, 0])):
		if (sommelier[i, 0] == 1):
			continue
		# mark as visited
		sommelier[i, 0] = 1
		nbr = find_neighbour_indices_without(wines, wines[i], eps, i)
		if (len(nbr) < min_pts):
			# mark as noise (cluster = -1)
			sommelier[i, 1] = -1
		else:
			C += 1
			# add to C cluster
			sommelier[i, 1] = C
			for w in nbr:
				if (sommelier[w, 0] == 0):
					sommelier[w, 0] = 1
					nbr1 = find_neighbour_indices_without(wines, wines[w], eps, w)
					if (len(nbr1) >= min_pts):
						merge_list2_to_list1(nbr, nbr1)
				if (sommelier[w, 1] == 0):
					sommelier[w, 1] = C
	
	clusters = []
	for i in range(C + 1):
		clusters.append([])
	for i in range(len(sommelier[:, 0])):
		if (sommelier[i, 1] == -1):
			clusters[0].append(i)
		else:
			clusters[sommelier[i, 1]].append(i)
	return clusters

def purity(wines, clusters):
	"""Calculating purity. Wine - etalon classes, clusters - result of clusterization"""
	N = len(wines[:, 0])
	s = 0
	for i in range(1, len(clusters)):
		clusters_in_etalon = {}
		for k in clusters[i]:
			if (wines[k][0] not in clusters_in_etalon):
				clusters_in_etalon[wines[k][0]] = 1
			else:
				clusters_in_etalon[wines[k][0]] += 1
		max_clust = 0
		for key in clusters_in_etalon:
			if (clusters_in_etalon[key] > max_clust):
				max_clust = clusters_in_etalon[key]
		s += max_clust
	s /= (N + 0.0)
	return s

def mutual_information(wines, clusters):
	"""Calculating mutual information. Wine - etalon classes, clusters - result of clusterization"""
	N = len(wines[:, 0])
	clusts_without_noise = list(clusters)
	clusts_without_noise.pop(0)
	C = len(clusts_without_noise)
	MI = 0
	for clust in clusts_without_noise:
		for etalon_class in range(1, C + 1):
			crossing = 0
			for wine_ind in clust:
				if (wines[wine_ind][0] == etalon_class):
					crossing += 1
			clust_size = len(clust)
			class_size = 0
			for wine in wines:
				if (wine[0] == etalon_class):
					class_size += 1
			if ((crossing != 0) and (clust_size != 0) and (class_size != 0)):
				MI += crossing * np.log2((N * crossing + 0.0) / (clust_size * class_size))
	MI = MI / N
	return MI

def rand_index(wines, clusters):
	"""Calculating rand index. Wine - etalon classes, clusters - result of clusterization"""
	N = len(wines[:, 0])
	clusts_without_noise = list(clusters)
	clusts_without_noise.pop(0)
	same_clusts = 0
	dif_clusts = 0
	for i in range(N):
		for j in range(i + 1, N):
			if (wines[i][0] == wines[j][0]):
				for clust in clusts_without_noise:
					if ((i in clust) and (j in clust)):
						same_clusts += 1
						break
			else:
				for clust in clusts_without_noise:
					if (((i in clust) and (j not in clust)) or ((i not in clust) and (j in clust))):
						dif_clusts += 1
						break;
	return (dif_clusts + same_clusts) / (scipy.special.binom(N, 2))


def test_on_two_dimens(file, eps, min_pts):
	"""Function applies dbscan to objects in file "file" and dbscan-parameters: eps and min_pts \
	Objects must be dots in two-dimensional space. After applying dbscan, function will show plot and \
	putiry, rand index and mutual information."""
	wines = get_lists_of_wines(file)
	clusters = dbscan(wines, eps, min_pts)
	color = []
	x = []
	y = []
	for i in range(len(clusters)):
		if (i == 0):
			for k in range(len(clusters[i])):
				color.append(0)
				x.append(wines[clusters[i][k], 1])
				y.append(wines[clusters[i][k], 2])
		else:
			for j in range(len(clusters[i])):
				color.append((i * 20) % 100)
				x.append(wines[clusters[i][j], 1])
				y.append(wines[clusters[i][j], 2])
	print clusters
	print "Purity = ", purity(wines, clusters)
	print "Mutual information = ", mutual_information(wines, clusters)
	print "Rand index = ", rand_index(wines, clusters)
	pl.scatter(x, y, c=color)
	pl.show()
	return purity

def test_on_main_task(file, eps, min_pts):
	"""Function applies dbscan to objects in file "file" and dbscan-parameters: eps and min_pts \
	Objects must be dots in any-dimensional space. After applying dbscan, function will print sizes of \
	all clusters and their objects and then purity, rand index and mutual information."""
	wines = get_lists_of_wines(file)
	clusters = dbscan(wines, eps, min_pts)
	for i in range(len(clusters)):
		if (i == 0):
			print "Noise:"
		else:
			print "Cluster = ", i, ":"
		for w in clusters[i]:
			print wines[w]
	for i in range(len(clusters)):
		if (i == 0):
			print "Noise size = ", len(clusters[i])
		else:
			print "Cluster ", i, " size = ", len(clusters[i])
	print "Purity = ", purity(wines, clusters)
	print "Mutual information = ", mutual_information(wines, clusters)
	print "Rand index = ", rand_index(wines, clusters)
	return purity

def main():
	test_on_main_task('./wine.data', 40, 5)


if __name__ == "__main__":
    main()