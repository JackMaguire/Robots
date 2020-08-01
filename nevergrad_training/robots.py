import os
os.environ["CUDA_DEVICE_ORDER"] = "PCI_BUS_ID"
os.environ["CUDA_VISIBLE_DEVICES"] = ""

from model_for_nn import *
import tensorflow as tf

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
import math

import sys
#import h5py

import nevergrad as ng

#import argparse

#np.random.seed( 0 )

#parser = argparse.ArgumentParser()
#parser.add_argument( "--model", help="filename for output file", default="model.h5", required=False )
#args = parser.parse_args()

#Board Input

def create_model():

    view_distance = 4
    n_board_states = 5 # OOB, empty, human, fire, robot (TODO reorder to match enum)
    in1_dim = 1 + (2*view_distance)
    input1 = Input(shape=(in1_dim,in1_dim,n_board_states,), name="in1", dtype="float32" )
    boardconv1 = Conv2D( filters=2, kernel_size=(1,1), padding='valid', data_format='channels_last', activation='relu' )( input1 )
    #print( boardconv1.shape )
    boardconv1 = LocallyConnected2D( filters=4, kernel_size=(3,3), strides=(2,2), padding='valid', data_format='channels_last', activation='relu' )( boardconv1 )
    #print( boardconv1.shape )
    #exit( 0 )
    boardconv1 = LocallyConnected2D( filters=5, kernel_size=(2,2), strides=(1,1), padding='valid', data_format='channels_last', activation='relu' )( boardconv1 )
    #boardconv1= BatchNormalization()(boardconv1)

    #Move Input
    n_move_channels = 5
    # 0: Is it save to move here? 0/1
    # 1: How many robots would die if I move here? ln(x)-1, -2 if 0
    # 2: Once I move here, is it safe to stay put? 0/1
    # 3: How many robots are in my line of sight here? ln(x)-1, -2 if 0
    # 4: (only in corners) How many robots are in this quadrant? log10(x)-1, -2 if 0
    # 5: (cardinal) How many robots are in this direction (sum of adjacent corners before nromalization)? log10(x) - 1, -2 if 0
    input2 = Input(shape=(3,3,n_board_states,), name="in2", dtype="float32" )
    layer = Conv2D( filters=10, kernel_size=(1,1), padding='valid', activation='relu' )( input2 )
    layer = Conv2D( filters=10, kernel_size=(1,1), padding='valid', activation='relu' )( layer )
    layer = Conv2D( filters=5, kernel_size=(1,1), padding='valid', activation='relu' )( layer )

    
    #This should be of shape( 3, 3, n_board_states + 5 )
    layer = tensorflow.keras.layers.concatenate( [boardconv1,layer], name="merge", axis=-1 )


    #in-place 1x1 conv
    #layer = LocallyConnected2D( filters=10, kernel_size=(1,1), padding='valid', data_format='channels_last', activation='relu' )( layer )

    #(2,2,N)
    layer = LocallyConnected2D( filters=7, kernel_size=(2,2), padding='valid', data_format='channels_last', activation='relu' )( layer )
    #print( layer.shape )

    layer = Flatten( data_format='channels_last' )( layer )

    input3 = Input(shape=(1,), name="in3", dtype="float32" )
    layer = tensorflow.keras.layers.concatenate( [input3,layer], name="merge_teleport", axis=-1 )
    
    layer = Dense( units=25, activation='relu' )( layer )
    #layer = Dense( units=100, activation='relu' )( layer )

    n_output = 11
    output = Dense( name="output", units=n_output )( layer )
    output = Softmax( name="softmax" )(output)

    model = Model(inputs=[input1, input2, input3], outputs=output )

    metrics_to_output=[ 'accuracy' ]
    model.compile( loss='categorical_crossentropy', optimizer='adam', metrics=metrics_to_output )
    #model.summary()

    initial_weights = model.get_weights()
    initial_weights = [np.random.permutation(w.flat).reshape(w.shape) for w in initial_weights]
    model.set_weights(initial_weights)

    return model

def parameterization_for_model( model ):

    #[print(weight.shape) for weight in model.trainable_weights]
    vars = []
    for weight in model.trainable_weights:
        vars.append( ng.p.Array( shape=tuple(weight.shape) ).set_bounds(-10, 10) )
    return ng.p.Instrumentation(*vars)

def run_single_simulation( model ):
    game = GamePtr()
    while True:
        observations = game.get_observations()
        board_input = np.asarray([observations[ 0 ]])
        local_input = np.asarray([observations[ 1 ]])
        n_tele = np.asarray( [[ game.n_safe_teleports_remaining() / 10.0 ]] )
        #print( board_input.shape, local_input.shape, n_tele.shape )
        #exit( 0 )
        n_tele.reshape( 1, 1 )
        
        decision = model.predict( [board_input,local_input,n_tele] ).flatten()
        assert( len(decision) == 11 )
        move = np.argmax( decision )
        
        game_over = False
        if move == 0: #Q
            game_over = game.move_human( -1, 1 )
        elif move == 1: #W
            game_over = game.move_human( 0, 1 )
        elif move == 2: #E
            game_over = game.move_human( 1, 1 )
        elif move == 3: #A
            game_over = game.move_human( -1, 0 )
        elif move == 4: #S
            game_over = game.move_human( 0, 0 )
        elif move == 5: #D
            game_over = game.move_human( 1, 0 )
        elif move == 6: #Z
            game_over = game.move_human( -1, -1 )
        elif move == 7: #X
            game_over = game.move_human( 0, -1 )
        elif move == 8: #C
            game_over = game.move_human( 1, -1 )
        elif move == 9: #T
            game_over = game.teleport()
        elif move == 10: #SPACE
            game_over = game.fast_cascade()

        if game_over:
            break
    
    return game.score()

def dump_model( weights, filename ):
    model = create_model()
    model.set_weights([tf.convert_to_tensor(arg, dtype=tf.float32) for arg in weights.args])
    model.save( filename )

def score_similation( weights, filename, score_to_beat ):
    model = create_model()
    model.set_weights([tf.convert_to_tensor(arg, dtype=tf.float32) for arg in weights.args])

    scores = np.zeros( 10 )
    #play 10 games
    for i in range( 0, 10 ):
        scores[ i ] = float( run_single_simulation( model ) )
        #perform transform
        if scores[ i ] != 0:
            scores[ i ] = -1.0 * math.log10( scores[ i ] )
            
    #return mean
    mean = np.mean( scores )
    if mean < score_to_beat:
        model.save( filename + "." + str( mean ) + ".h5" )
    return mean
