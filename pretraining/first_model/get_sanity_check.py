import os
os.environ["CUDA_DEVICE_ORDER"] = "PCI_BUS_ID"
os.environ["CUDA_VISIBLE_DEVICES"] = ""

from model_for_nn import *

#from tensorflow.keras import *
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras import metrics

from tensorflow.keras.models import load_model
import tensorflow.keras.backend as K
import tensorflow.keras.callbacks
import tensorflow.keras
import numpy
import numpy as np

import sys
#sys.path.append("/nas/longleaf/home/jackmag")#for h5py
import h5py

#import pandas as pd

import argparse
import random
import time
import subprocess

from tensorflow.keras import backend as K
import tensorflow as tf

from tensorflow.python.util.tf_export import tf_export


########
# INIT #
########

numpy.random.seed( 0 )

parser = argparse.ArgumentParser()
parser.add_argument( "--model", help="Most recent model file", required=True )
#parser.add_argument( "--out", help="Filename for result", required=True )
args = parser.parse_args()

        
#########
# START #
#########


if os.path.isfile( args.model ):
    model = load_model( args.model )
else:
    print( "Model " + args.model + " is not a file" )
    exit( 1 )


in1 = []
in2 = []
out = []

thingy = sanity_check_values()
if len( thingy ) == 3:
    in1.append( thingy[ 0 ] )
    in2.append( thingy[ 1 ] )
    out.append( thingy[ 2 ] )

in1arr = np.asarray( in1 )
in2arr = np.asarray( in2 )
outarr = np.asarray( out )

preds = model.predict( [in1arr,in2arr] )
print(preds)
