import logging
from gym.envs.registration import register

logger = logging.getLogger(__name__)

register(
    id='DeepChase-v0',
    entry_point='DeepChase.envs:DeepChaseEnv',
    max_episode_steps=200,
)
