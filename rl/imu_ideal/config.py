"""Central configuration for sesameRL (mjlab + rsl_rl).

Everything experiment-relevant lives here. Toggle a term on or off with its
`enabled` flag; tune weights, params or curriculum stages in place.
"""

from __future__ import annotations

import math

# ---------------------------------------------------------------------------
# Robot -- Sesame has 8 revolute joints (hip + knee per leg). Leg mapping:
#   L3 = Front-Left    L4 = Rear-Left
#   R3 = Front-Right   R4 = Rear-Right
# ---------------------------------------------------------------------------
URDF_PATH = "urdf_model/Sesame.urdf"
BASE_LINK_NAME = "base_link"

HIP_JOINT_REGEX = "Joint_[LR][12]"
KNEE_JOINT_REGEX = "Joint_[LR][34]"

# The URDF ships asymmetric joint limits: four joints use [0, pi/2], the
# other four use [pi/2, pi]. The neutral stand pose is the midpoint of each
# hip range, and ~0.2 rad inside the extended endpoint for knees so the
# feet just reach the floor at BASE_INIT_POS[2]=0.08.
_HIP_LOW_MID = math.pi / 4              # joints with range [0, pi/2]
_HIP_HIGH_MID = 3 * math.pi / 4         # joints with range [pi/2, pi]
_KNEE_LOW_EXT = math.pi / 4             # knees  with range [0, pi/2]  (near 0)
_KNEE_HIGH_EXT = 3 * math.pi / 4        # knees  with range [pi/2, pi] (near pi)

DEFAULT_JOINT_ANGLES = {
    "Joint_L1": _HIP_LOW_MID,
    "Joint_L2": _HIP_HIGH_MID,
    "Joint_R1": _HIP_HIGH_MID,
    "Joint_R2": _HIP_LOW_MID,
    "Joint_L3": _KNEE_LOW_EXT,
    "Joint_L4": _KNEE_HIGH_EXT,
    "Joint_R3": _KNEE_HIGH_EXT,
    "Joint_R4": _KNEE_LOW_EXT,
}

BASE_INIT_POS = (0.0, 0.0, 0.06)

# Actuator gains (PD servo per joint). effort/velocity limits come from the
# URDF; stiffness/damping tuned so action_scale ~= 0.4 rad.
EFFORT_LIMIT = 0.215      # Nm
VELOCITY_LIMIT = 10.47    # rad/s

# ![TODO] Tuned somewhat manually, open to edit
STIFFNESS = 0.14
DAMPING = 0.003
ARMATURE = 1e-5

# Per-actuator action scale. q_target = default_pose + scale * action.
# ?[OPT] Didn't want to fully extend the legs
ACTION_SCALE = 0.5 #0.25 * EFFORT_LIMIT / STIFFNESS


# ---------------------------------------------------------------------------
# Sim-to-real fidelity  (hardware-realistic defaults — toggle for ablations)
# ---------------------------------------------------------------------------

# Actuator delay: motorCurrentDelay=40ms on hardware; dt=5ms → 8 physics steps.
# Widen to [8, 16] to cover the full sequential 8-servo dispatch spread (40–80ms).
ACTUATOR_DELAY_ENABLED: bool = True
ACTUATOR_DELAY_MIN_PHYSICS_STEPS: int = 8   # 40 ms nominal
ACTUATOR_DELAY_MAX_PHYSICS_STEPS: int = 8   # set to 16 for spread randomization

# DC motor torque-speed curve (off by default — changes actuator type, retrain required).
ACTUATOR_DC_MOTOR_ENABLED: bool = False
DC_MOTOR_SATURATION_EFFORT: float = EFFORT_LIMIT    # MG90S stall torque ≈ 0.215 Nm
DC_MOTOR_VELOCITY_LIMIT: float = VELOCITY_LIMIT     # MG90S no-load speed ≈ 10.47 rad/s

# joint_pos: only last commanded angle is known on hardware (open-loop servo).
# Delay in CONTROL STEPS (1 step = decimation * dt = 20 ms).
# 2–4 control steps covers the 40–80 ms sequential servo sweep.
OBS_JOINT_POS_DELAY_ENABLED: bool = True
OBS_JOINT_POS_DELAY_MIN_STEPS: int = 2
OBS_JOINT_POS_DELAY_MAX_STEPS: int = 4
OBS_JOINT_POS_NOISE_STD: float = 0.05    # rad (~3°; typical open-loop servo error)

# joint_vel: no velocity sensor on hardware — disable for a deployable policy.
OBS_JOINT_VEL_ENABLED: bool = False

# base_lin_vel: estimated by integrating MPU6050 accelerometer (after gravity removal
# via DMP orientation). Real estimate drifts at ~0.2 m/s RMS; modelled below.
OBS_BASE_LIN_VEL_ENABLED: bool = True

# IMU noise calibrated to MPU6050 datasheet: gyro ~0.005 rad/s, accel ~0.05 m/s².
IMU_NOISE_ENABLED: bool = True
IMU_GYRO_NOISE_STD: float = 0.005    # rad/s
IMU_ACCEL_NOISE_STD: float = 0.004   # dimensionless (0.05 m/s² / 9.81 ≈ 0.005)
IMU_LIN_VEL_NOISE_STD: float = 0.2   # m/s; models accelerometer-integration drift


# ---------------------------------------------------------------------------
# Scene / simulation
# ---------------------------------------------------------------------------
NUM_ENVS = 4096 # ?[OPT] Check your VRAM, 8192 envs almost fills 8GB 
EPISODE_LENGTH_S = 20
FALL_ANGLE_DEG = 60.0          # episode termination tilt threshold
TERMINATE_ON_FALL = True


# ---------------------------------------------------------------------------
# Observations. The `actor` and `critic`
# observation groups both see the same set; only the actor group gets the
# configured observation-noise corruption.
# ---------------------------------------------------------------------------
# ?[OPT] No need for observation history, this seems fine but open to edits
OBSERVATIONS = {
    "base_lin_vel":      {"enabled": OBS_BASE_LIN_VEL_ENABLED},
    "base_ang_vel":      {"enabled": True},
    "projected_gravity": {"enabled": True},
    "joint_pos":         {"enabled": True},  # delta from default pose
    "joint_vel":         {"enabled": OBS_JOINT_VEL_ENABLED},
    "actions":           {"enabled": True},  # previous action
    "command":           {"enabled": True},  # (vx, vy, wz) target
}


# ---------------------------------------------------------------------------
# Rewards -- weights are passed straight to mjlab's RewardManager.
# ---------------------------------------------------------------------------
# ![TODO] Play with rewards
REWARDS = {
    "track_linear_velocity":  {"enabled": True, "weight":  2.0},
    "track_angular_velocity": {"enabled": True, "weight":  2.0},
    "upright":                {"enabled": True, "weight":  1.0},
    "action_rate_l2":         {"enabled": True, "weight": -0.1},
    "foot_slip":              {"enabled": True, "weight": -0.05},
    "dof_pos_limits":         {"enabled": True, "weight": -1.0},
    "soft_landing":           {"enabled": True, "weight": -1e-5},
    "air_time":               {"enabled": True, "weight":  0.5, "cmd_threshold": 0.1},

    # sum(max(tau*qdot, 0)) over all 8 joints; discourages thrashing.
    "electrical_power":       {"enabled": True, "weight": -2e-5},
}


# ---------------------------------------------------------------------------
# Events (domain randomization & perturbations)
# ---------------------------------------------------------------------------
# ![TODO] Play with these
EVENTS = {
    "push_robot":    {"enabled": False},
    "encoder_bias":  {"enabled": True},
    "foot_friction": {"enabled": True},
    "base_com":      {"enabled": True},
}


# ---------------------------------------------------------------------------
# Commands -- UniformVelocityCommand over (lin_vel_x, lin_vel_y, ang_vel_z).
# iteration*24, where iteration is the number of steps seen in tensorboard
# 24 = num_steps_per_env in PPO config
# ![TODO] Currently trying out some curriculums, edit as you wish
# ---------------------------------------------------------------------------
CURRICULUM_ENABLED = True
COMMAND_CURRICULUM = [
    # Stage 0: forward/backward only, narrow range. Easy warmup.
    {"step":        0, "x_com": (-0.3, 0.3), "y_com": ( 0.0, 0.0), "z_com": ( 0.0, 0.0)},
    # Stage 1: widen forward/backward. More time here before adding axes.
    {"step":   500*24, "x_com": (-0.5, 0.5), "y_com": ( 0.0, 0.0), "z_com": ( 0.0, 0.0)},
    # Stage 2: add lateral strafe. Still no yaw — robot must learn straight walking first.
    {"step":  2000*24, "x_com": (-0.5, 0.5), "y_com": (-0.3, 0.3), "z_com": ( 0.0, 0.0)},
    # Stage 3: introduce gentle yaw. Capped at ±0.5 rad/s — realistic for MG90S legs.
    {"step":  3500*24, "x_com": (-0.5, 0.5), "y_com": (-0.3, 0.3), "z_com": (-0.5, 0.5)},
]
REL_STANDING_ENVS = 0.2


# ---------------------------------------------------------------------------
# Training (PPO via rsl_rl)
# ---------------------------------------------------------------------------
TASK_ID = "Sesame-Velocity-Flat"
EXPERIMENT_NAME = "sesame_velocity"
MAX_ITERATIONS = 10_000
SEED = 1

PPO = {
    "num_steps_per_env": 24,
    "save_interval": 100,
    "logger": "tensorboard",

    "algorithm": {
        "value_loss_coef": 0.5,
        "use_clipped_value_loss": True,
        "clip_param": 0.2,
        "entropy_coef": 0.01,
        "num_learning_epochs": 5,
        "num_mini_batches": 4,
        "learning_rate": 1.0e-3,
        "schedule": "adaptive",
        "gamma": 0.99,
        "lam": 0.95,
        "desired_kl": 0.01,
        "max_grad_norm": 1.0,
    },

    # Actor runs on the ESP32: (64,128,64) → ~19k params, ~75 KB (fits internal SRAM).
    # Critic is training-only: kept larger for better value estimation.
    "actor": {
        "hidden_dims": (64, 128, 64),
        "activation": "elu",
        "obs_normalization": False,
        "distribution_cfg": {
            "class_name": "GaussianDistribution",
            "init_std": 0.5,
            "std_type": "scalar",
        },
    },
    "critic": {
        "hidden_dims": (256, 512, 256, 128),
        "activation": "elu",
        "obs_normalization": False,
    },
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def enabled_rewards() -> dict:
    return {k: v for k, v in REWARDS.items() if v.get("enabled", True)}


def enabled_observations() -> list[str]:
    return [k for k, v in OBSERVATIONS.items() if v.get("enabled", True)]


def enabled_events() -> list[str]:
    return [k for k, v in EVENTS.items() if v.get("enabled", True)]
