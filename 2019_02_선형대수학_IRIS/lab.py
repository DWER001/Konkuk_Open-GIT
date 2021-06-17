# 0. Setting Lab
import pandas
from sklearn.preprocessing import LabelEncoder
from sklearn.model_selection import train_test_split
from keras.models import Sequential
from keras.layers import Dense
from keras.optimizers import Adam

iteration_range = [0.1, 0.2, 0.3] # test range (0~1)
Node = 32 # 0 < Node < 64
Layer = 3 # 1 < Layer < 5
Epoch = 50 # 0 < Epoch < 200


# 1. Load Data
data = pandas.read_csv("irisData.csv")


# 2. Label Encoding
sepal = data.iloc[:,0:4].values
species = data.iloc[:,4].values
species = pandas.get_dummies(LabelEncoder().fit_transform(species)).values


# 3. Prepare Train / Test Set
iteration = len(iteration_range)
sepal_train = []
sepal_test = []
species_train = []
species_test = []

for i in range(iteration):
	x_train, x_test, y_train, y_test = train_test_split(sepal, species, test_size=iteration_range[i], random_state=1) 
	
	sepal_train.append(x_train)
	sepal_test.append(x_test)
	species_train.append(y_train)
	species_test.append(y_test)

#for i in range(iteration):
#	print(sepal_train[i].shape, sepal_test[i].shape, species_train[i].shape, species_test[i].shape)


# 4. Make Train Model
model = Sequential()
model.add(Dense(Node,input_shape=(4,),activation='relu'))
for i in range(Layer - 3 + 1):
	model.add(Dense(Node,activation='relu'))
model.add(Dense(3,activation='softmax'))
model.compile(loss='categorical_crossentropy', optimizer='Adam', metrics=['accuracy'])

#model.summary()


# 5. Training & Result
for i in range(iteration):
	model.fit(sepal_train[i], species_train[i], validation_data=(sepal_test[i], species_test[i]), epochs=Epoch)
	loss, accuracy = model.evaluate(sepal_test[i], species_test[i])
	
	test = int(iteration_range[i] * 100)
	train = int(100-test)
	print("Train : {}%, Test : {}% >> Accuracy {:.4f}".format(train, test, accuracy))
