import numpy as np
import math

import gym
from gym import spaces
from gym.utils import seeding

import model_for_nn as robots_game

#https://stackoverflow.com/questions/54022606/openai-gym-how-to-create-one-hot-observation-space
'''
class OneHotEncoding(gym.Space):
    """
    {0,...,1,...,0}

    Example usage:
    self.observation_space = OneHotEncoding(size=4)
    """
    def __init__(self, size=None):
        assert isinstance(size, int) and size > 0
        self.size = size
        gym.Space.__init__(self, (), np.int64)

    def sample(self):
        one_hot_vector = np.zeros(self.size)
        one_hot_vector[np.random.randint(self.size)] = 1
        return one_hot_vector

    def contains(self, x):
        if isinstance(x, (list, tuple, np.ndarray)):
            number_of_zeros = list(x).contains(0)
            number_of_ones = list(x).contains(1)
            return (number_of_zeros == (self.size - 1)) and (number_of_ones == 1)
        else:
            return False

    def __repr__(self):
        return "OneHotEncoding(%d)" % self.size

    def __eq__(self, other):
        return self.size == other.size
'''

class DeepChaseGym(gym.Env):
    def __init__(self):
        #self.action_space = spaces.OneHotEncoding( size=11 )
        self.game = robots_game.GamePtr()
        
        self.action_space = spaces.Discrete( 11 )

        self.ob_space1 = spaces.Box( low=-1, high=np.inf, shape=(9,9,5) )
        self.ob_space2 = spaces.Box( low=-1, high=np.inf, shape=(3,3,5) )
        self.observation_space = spaces.Tuple( self.ob_space1, self.ob_space2 )

        self.seed()
        self.reset()

    def seed(self, seed=None):
        return [seed]

    def step(self, action):
        '''
        if not self.action_space.contains(action):
            print( action )
            print( self.action_space )
            print( self.action_space.contains( [action] ) )
            assert self.action_space.contains(action)
        '''

        print( action )
        print( action.shape ) #hoping for (1,11)

        # 1 INDEXED!
        move = 1 #TODO
        
        done = False
        if move == 1:
            # Q
            done = self.game.move_human( -1, 1 )
        elif move == 2:
            # W
            done = self.game.move_human( 0, 1 )
        elif move == 3:
            # E
            done = self.game.move_human( 1, 1 )
        elif move == 4:
            # A
            done = self.game.move_human( -1, 0 )
        elif move == 5:
            # S
            done = self.game.move_human( 0, 0 )
        elif move == 6:
            # D
            done = self.game.move_human( 1, 0 )
        elif move == 7:
            # Z
            done = self.game.move_human( -1, -1 )
        elif move == 8:
            # X
            done = self.game.move_human( 0, -1 )
        elif move == 9:
            # C
            done = self.game.move_human( 1, -1 )
        elif move == 10:
            # T
            done = self.game.teleport()
        elif move == 11:
            # SPACE
            done = self.game.fast_cascade()


        reward = cal_reward()
        
        print( action, self.number, reward )

        self.guess_count += 1
        done = self.guess_count >= self.guess_max

        return get_observation(), reward, done, {}

    def calc_reward(self):
        #            Max of 0.5       Starts at 0, no upper bound
        #         ----------------    ------------------------
        # f(x) = ( min(10,R) / 20 ) + if(R>10){ (log10(R) - 1)/5 }   where R is # robots killed
        #
        # R    Score
        # 0     0
        # 1     0.05
        # 2     0.1
        # 5     0.25
        # 10    0.5
        # 11    0.508
        # 100   0.7
        # 10k   1.1
        # 22110 1.17  #max score

        # Maybe raise this to an exponent if it is too shallow on the high ends?
        R = self.game.score() #n robots killd
        reward = float(min( 10.0, R )) / 20.0
        if R > 10.0:
            reward += (math.log10(R) - 1.0)/5.0
        return reward
        
    
    def get_observation(self):
        observations = robots_game.get_observations( self.game )
        return observations
    
    def reset(self):
        self.game.reset()
        return get_observation()
