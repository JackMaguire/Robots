#!/bin/bash

python3 pretrain.py --data ../../src/training_data/all_data
python3 convert_h5_to_pb.py --model model.h5
python3 ~/smartdesign/utilities/visualize_model.py model.h5
python3 get_sanity_check.py --model model.h5 2>/dev/null > sanity_check.txt

git add sanity_check.txt model.h5 model.h5.png saved_model
