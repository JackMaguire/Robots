import os
os.environ["CUDA_DEVICE_ORDER"] = "PCI_BUS_ID"
os.environ["CUDA_VISIBLE_DEVICES"] = ""


#import tensorflow as tf

#from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import *
from tensorflow.keras import metrics
from tensorflow.keras import optimizers

from tensorflow.keras.models import load_model
from tensorflow.keras.models import Model
import tensorflow.keras.backend as K
import tensorflow.keras.callbacks
import tensorflow.keras
import numpy

import sys
#import h5py

import argparse

numpy.random.seed( 0 )

parser = argparse.ArgumentParser()
parser.add_argument( "--model", help="filename for output file", default="model.h5", required=False )
args = parser.parse_args()

#Board Input
n_board_states = 4 # empty, human, fire, robot (TODO reorder to match enum)
n_board_states += 2 # dx / 10, dy / 10
input1 = Input(shape=(45,30,n_board_states,), name="in1", dtype="float32" )
boardconv1 = Conv2D( filters=100, kernel_size=(3,3), padding='same', data_format='channels_last', activation='relu' )( input1 )
print( boardconv1.shape ) #(None, 45, 30, 100)
boardconv1 = Conv2D( filters=150, kernel_size=(3,3), strides=(3,3), padding='valid', data_format='channels_last', activation='relu' )( boardconv1 )
print( boardconv1.shape ) #(None, 15, 10, 200)
boardconv1 = Conv2D( filters=50, kernel_size=(5,5), strides=(5,5), padding='valid', data_format='channels_last', activation='relu' )( boardconv1 )
print( boardconv1.shape ) #(None, 3, 2, 200)
#print( boardconv1.shape[ 1 ] )

flatboard = Flatten( data_format='channels_last' )( boardconv1 )

#exit(1)

#Move Input
n_move_channels = 5
# 0: Is it save to move here? 0/1
# 1: How many robots would die if I move here? ln(x)-1, -2 if 0
# 2: Once I move here, is it safe to stay put? 0/1
# 3: How many robots are in my line of sight here? ln(x)-1, -2 if 0
# 4: (only in corners) How many robots are in this quadrant? log10(x)-1, -2 if 0
# 5: (cardinal) How many robots are in this direction (sum of adjacent corners before nromalization)? log10(x) - 1, -2 if 0
input2 = Input(shape=(3,3,n_board_states,), name="in2", dtype="float32" )

#in-place 1x1 conv
layer = Conv2D( filters=20, kernel_size=(1,1), padding='valid', data_format='channels_last', activation='relu' )( input2 )

#(2,2,N)
layer = LocallyConnected2D( filters=15, kernel_size=(2,2), padding='valid', data_format='channels_last', activation='relu' )( layer )
print( layer.shape )



layer = Flatten( data_format='channels_last' )( layer )
layer = tensorflow.keras.layers.concatenate( [layer,flatboard], name="merge", axis=-1 )

#10
layer = Dense( units=50, activation='relu' )( layer )

n_output = 11
output = Dense( name="output", units=n_output, activation='softmax' )( layer )

model = Model(inputs=[input1, input2], outputs=output )

metrics_to_output=[ 'accuracy' ]
model.compile( loss='binary_crossentropy', optimizer='adam', metrics=metrics_to_output )
model.save( args.model + ".h5" )
model.summary()
