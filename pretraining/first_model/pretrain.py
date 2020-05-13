import os
os.environ["CUDA_DEVICE_ORDER"] = "PCI_BUS_ID"
os.environ["CUDA_VISIBLE_DEVICES"] = ""

from model_for_nn import *
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
import numpy as np

import sys
#import h5py

import argparse

np.random.seed( 0 )

parser = argparse.ArgumentParser()
parser.add_argument( "--model", help="filename for output file", default="model.h5", required=False )
parser.add_argument( "--data", help="filename for data file", required=True )
args = parser.parse_args()

#Board Input
view_distance = 4
n_board_states = 5 # OOB, empty, human, fire, robot (TODO reorder to match enum)
in1_dim = 1 + (2*view_distance)
input1 = Input(shape=(in1_dim,in1_dim,n_board_states,), name="in1", dtype="float32" )
boardconv1 = Conv2D( filters=15, kernel_size=(3,3), padding='valid', data_format='channels_last', activation='relu' )( input1 )
print( boardconv1.shape ) #(None, 7, 7, N)
print( boardconv1.shape[ 1 ] )
while( boardconv1.shape[ 1 ] > 3 ):
    boardconv1 = Conv2D( filters=10, kernel_size=(3,3), padding='valid', data_format='channels_last', activation='relu' )( boardconv1 )
    print( boardconv1.shape )

#Move Input
n_move_channels = 5
# 0: Is it save to move here? 0/1
# 1: How many robots would die if I move here? ln(x)-1, -2 if 0
# 2: Once I move here, is it safe to stay put? 0/1
# 3: How many robots are in my line of sight here? ln(x)-1, -2 if 0
# 4: (only in corners) How many robots are in this quadrant? log10(x)-1, -2 if 0
# 5: (cardinal) How many robots are in this direction (sum of adjacent corners before nromalization)? log10(x) - 1, -2 if 0
input2 = Input(shape=(3,3,n_board_states,), name="in2", dtype="float32" )

#This should be of shape( 3, 3, n_board_states + 5 )
layer = tensorflow.keras.layers.concatenate( [boardconv1,input2], name="merge", axis=-1 )


#in-place 1x1 conv
layer = LocallyConnected2D( filters=20, kernel_size=(1,1), padding='valid', data_format='channels_last', activation='relu' )( layer )

#(2,2,N)
layer = LocallyConnected2D( filters=15, kernel_size=(2,2), padding='valid', data_format='channels_last', activation='relu' )( layer )
print( layer.shape )

layer = Flatten( data_format='channels_last' )( layer )

#10
layer = Dense( units=50, activation='relu' )( layer )

n_output = 11
output = Dense( name="output", units=n_output )( layer )
output = Softmax( name="softmax" )(output)

model = Model(inputs=[input1, input2], outputs=output )

metrics_to_output=[ 'accuracy' ]
model.compile( loss='categorical_crossentropy', optimizer='adam', metrics=metrics_to_output )
model.summary()

in1 = []
in2 = []
out = []

with open( args.data ) as f:
    for line in f:
        line2 = line.strip()
        n = 0
        for a in [ True, False ]:
            for b in range( 0, 4 ):
                thingy = parse_string( line2, a, b )
                in1.append( thingy[ 0 ] )
                in2.append( thingy[ 1 ] )
                out.append( thingy[ 2 ] )
                n += 1
        assert n == 8

in1arr = np.asarray( in1 )
in2arr = np.asarray( in2 )
outarr = np.asarray( out )

print( in1arr.shape )
print( in2arr.shape )
print( outarr.shape )
#exit( 0 )

csv_logger = tensorflow.keras.callbacks.CSVLogger( "training_log.csv", separator=',', append=False )
stop = tensorflow.keras.callbacks.EarlyStopping(monitor='val_loss', min_delta=0, patience=10, verbose=0, mode='min', baseline=None, restore_best_weights=True)
lrer = tensorflow.keras.callbacks.ReduceLROnPlateau(monitor='val_loss', factor=0.5, patience=5, verbose=0, mode='auto', min_delta=0.001, cooldown=0, min_lr=0)

callbacks=[csv_logger,stop,lrer]

        
model.fit( x=[in1arr,in2arr], y=outarr, batch_size=64, epochs=300, verbose=1, callbacks=callbacks, shuffle=True, validation_split=0.25 )
model.save( args.model )
