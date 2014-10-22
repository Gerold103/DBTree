test.data - big amount of dots. In each string: furst number - etalon class, second and third - x and y
test2.data - several dots, representing three classes, which are very easy to find by dbscan
wine.data - original data set

test_on_two_dimens() function can be figured out on test.data and test2.data
test_on_main_task() function can be figured on any data set

Program doesn't use any constants for number of etalon classes or number of objects, so you can change by any way: dimensionality of dots, number of dots, number of etalon classes without changing program. But remember, that test_on_two_dimens() can by figured out only on two-dimensional data sets.