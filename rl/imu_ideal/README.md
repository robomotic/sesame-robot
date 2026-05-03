# sesameRL

RL training environment for the 4-legged, 8-DOF [Sesame quadruped,](https://github.com/dorianborian/sesame-robot/) built on [mjlab](https://mujocolab.github.io/mjlab/). PPO via `rsl_rl`, TensorBoard logging, and the [Sesame URDF](urdf_model/Sesame.urdf) loaded directly into a MuJoCo `MjSpec`.

Every observation, reward, event, curriculum stage, and PPO hyperparameter lives in the [config.py](config.py).

---

## Quick start

- Download the repository
- Create and activate a UV virtual environment:
  ```bash
  uv venv          # creates .venv directory
  source .venv/bin/activate  # on Windows: .venv\Scripts\activate
  ```
- Install mjlab (includes PyTorch and other dependencies):
  ```bash
  uv pip install git+https://github.com/mujocolab/mjlab.git
  ```
- From the base folder, with the virtual environment active:

```bash
python validate.py                                          # sinusoidal joint drive, no policy
python train.py                                             # full PPO run
python play.py                                              # auto-loads latest checkpoint
python play.py --checkpoint-file logs/<run>/model_<n>.pt    # play a specific checkpoint
tensorboard --logdir logs                                   # monitor training
```

## Configuration

All experiment configurations are in [`config.py`](config.py). The file is organized into sections:

- **Robot**: URDF paths, joint mappings, default poses, actuator gains
- **Sim-to-real fidelity**: Hardware-realistic defaults (actuator delay, motor model, sensor noise)
- **Scene / simulation**: Number of parallel environments, episode length, termination conditions
- **Observations**: Which sensor readings are fed to the policy (can enable/disable for sim-to-real transfer)
- **Rewards**: Reward functions and weights for the velocity-tracking task
- **Events**: Domain randomization and perturbations (push robot, encoder bias, etc.)
- **Commands**: Velocity command curriculum for progressive learning
- **Training (PPO)**: Hyperparameters for the rsl_rl PPO trainer
- **Helpers**: Functions to get enabled configs

Key configuration flags for sim-to-real transfer:
- `OBS_JOINT_VEL_ENABLED`: Set to `False` to disable velocity observations (not available on real hardware)
- `OBS_BASE_LIN_VEL_ENABLED`: Set to `False` to disable linear velocity observations (MPU6050 limitation)
- `ACTUATOR_DC_MOTOR_ENABLED`: Set to `True` to use realistic DC motor model (requires retraining)
- `OBS_JOINT_POS_DELAY_ENABLED`: Set to `True` to simulate open-loop servo delay
- `IMU_NOISE_ENABLED`: Set to `True` to add IMU noise matching MPU6050 specs

### Simulation Timesteps & Control Frequency

The simulation uses a two-level timing approach based on the comments in config.py:

1. **Physics Timestep (`dt`)**: 5ms (0.005s) - defined in the base mjlab velocity config as `mujoco.timestep = 0.005`
2. **Decimation**: 4 - meaning the simulation advances 4 physics steps per control step
3. **Control Frequency**: 50Hz - calculated as `1 / (dt * decimation) = 1 / (0.005 * 4) = 50Hz`

This means:
- Each control step (policy inference) corresponds to 20ms of simulated time
- The policy runs at 50Hz (every 20ms)
- Within each control step, the physics simulator runs 4 sub-steps of 5ms each
- Actuator delays are specified in physics steps: 40ms hardware delay = 8 physics steps at 5ms/dt

These values can be verified in:
- `env_cfg.py`: Uses `sesame_flat_env_cfg()` which builds on the base velocity config
- `config.py`: Lines 63-75 document the timing relationships
- `validate.py`: Line 79 shows `dt = env_cfg.decimation * env_cfg.sim.mujoco.timestep` calculation

## Running Experiments

### Training

Run a standard PPO training experiment:

```bash
python train.py
```

This will:
- Use the experiment name from `config.EXPERIMENT_NAME` ("sesame_velocity")
- Run for `config.MAX_ITERATIONS` (10,000) PPO iterations
- Use `config.NUM_ENVS` (4096) parallel environments
- Save logs to `logs/<timestamp>/` and checkpoints every 100 iterations
- Use seed `config.SEED` (1) for reproducibility

### Customizing Training

Override any config parameter via command line:

```bash
# Smoke test with fewer iterations
python train.py --max-iterations 20

# Custom experiment name
python train.py --run-name my_first_run

# Reduce parallel environments (for limited VRAM)
python train.py --num-envs 1024

# Change random seed
python train.py --seed 42

# Combine multiple overrides
python train.py --max-iterations 500 --run-name test --num-envs 2048
```

### Evaluation / Visualization

View the latest trained policy:

```bash
python play.py
```

This auto-loads the newest checkpoint from `logs/<timestamp>/` and runs it with the Viser web viewer.

To load a specific checkpoint:

```bash
python play.py --checkpoint-file logs/2026-05-03_10-30-00/model_500.pt
```

Use the native MuJoCo viewer instead of Viser:

```bash
python play.py --viewer native
```

### Monitoring Training

Launch TensorBoard to view training metrics:

```bash
tensorboard --logdir logs
```

This will show:
- Reward curves (episode reward, individual reward components)
- Policy losses (actor/critic loss, entropy, KL divergence)
- Command tracking performance
- Episode length and success statistics

## Experiment Workflow

1. **Modify config.py** to adjust robot/sim settings, rewards, or curriculum
2. **Run training**: `python train.py [--max-iterations N] [--run-name NAME]`
3. **Monitor progress**: `tensorboard --logdir logs`
4. **Evaluate**: `python play.py` or `python play.py --checkpoint-file PATH/TO/MODEL.PT`
5. **Iterate**: Adjust config based on results and repeat

---

## Layout

```
sesameRL/
├── validate.py        sanity check
├── config.py          all the configs, main place to edit
├── train.py           run for training
├── play.py            run for sandbox play
├── sesame_robot.py    imports robot from URDF file
├── env_cfg.py         composes mjlab environment from config.py
├── urdf_model/        Sesame.urdf + STL meshes
├── images/            visualization assets
└── logs/              rsl_rl run dirs (TensorBoard + checkpoints + params snapshots)
```

## Visualization

The `images/` directory contains visualizations of the robot and training results.

![Sesame Quadruped](images/image.png)

---

## Robot notes

Link / joint naming inherited from the Sesame project:

| Link    | Role         | Hip joint | Knee joint |
|---------|--------------|-----------|------------|
| Link_L3 | Front-left   | Joint_L1  | Joint_L3   |
| Link_L4 | Rear-left    | Joint_L2  | Joint_L4   |
| Link_R3 | Front-right  | Joint_R1  | Joint_R3   |
| Link_R4 | Rear-right   | Joint_R2  | Joint_R4   |

---

## Roadmap

- [ ] URDF checks
	- [ ] Solidworks->URDF is not very mature, so some positions/axes might be off
	- [X] Starting orientation is garbage visually. Has no adverse effect, but would be nice if it was standing straight.
- [ ] Check the [TODO] and [OPT] tags in [config.py](config.py)
    - [ ] Tune kp-kd values
    - [ ] Make the NN small enough to fit ESP32
    - [ ] Current observations are not realizable with existing hardware
    - [ ] Currently uses curriculum, can upgrade to hierarchical learning
    - [ ] Play with rewards for better locomotion
- [ ] Terrain is flat, can upgrade to a rough terrain
- [ ] Sim-to-real preparation (needs extended hardware)

## Credits
Original concept and skeleton code from @karabibik