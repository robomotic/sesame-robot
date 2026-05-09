"""Replay pre-programmed gait sequences from firmware/movement-sequences.h.

Translates servo angle keyframes into MuJoCo simulation actions and steps
the environment forward, using the same viewer stack as validate.py.

Motor ID → URDF joint mapping (firmware ServoName enum + web UI labels):
  S0 / R1 (index 0) → Joint_R1    S1 / R2 (index 1) → Joint_R2
  S2 / L1 (index 2) → Joint_L1    S3 / L2 (index 3) → Joint_L2
  S4 / R4 (index 4) → Joint_R4    S5 / R3 (index 5) → Joint_R3
  S6 / L3 (index 6) → Joint_L3    S7 / L4 (index 7) → Joint_L4

Usage:
  python sequences.py                          # cycle through all sequences
  python sequences.py --sequence walk
  python sequences.py --sequence dance --viewer native
  python sequences.py --cycles 5              # walk/turn loop count
  python sequences.py --list                  # print names and exit
"""

from __future__ import annotations

import argparse
import math
from typing import NamedTuple

import torch

from mjlab.envs import ManagerBasedRlEnv
from mjlab.rl import RslRlVecEnvWrapper
from mjlab.viewer import NativeMujocoViewer, ViserPlayViewer

import config as C
from env_cfg import sesame_flat_env_cfg


# ---------------------------------------------------------------------------
# Motor ID ↔ URDF joint mapping
# ---------------------------------------------------------------------------

SERVO_TO_JOINT: dict[int, str] = {
    0: "Joint_R1",
    1: "Joint_R2",
    2: "Joint_L1",
    3: "Joint_L2",
    4: "Joint_R4",
    5: "Joint_R3",
    6: "Joint_L3",
    7: "Joint_L4",
}

_JOINT_TO_SERVO: dict[str, int] = {v: k for k, v in SERVO_TO_JOINT.items()}

# Firmware servo name → index (matches ServoName enum)
_NAME_TO_IDX: dict[str, int] = {
    "R1": 0, "R2": 1, "L1": 2, "L2": 3,
    "R4": 4, "R3": 5, "L3": 6, "L4": 7,
}


# ---------------------------------------------------------------------------
# Keyframe data model
# ---------------------------------------------------------------------------

class Keyframe(NamedTuple):
    """Complete servo state held for duration_ms milliseconds.

    angles maps servo index (0-7) to degrees [0, 180], mirroring the
    firmware's setServoAngle(channel, angle) convention.
    """
    angles: dict[int, int]
    duration_ms: int


def _all_90() -> dict[int, int]:
    return {i: 90 for i in range(8)}


def _stand_state() -> dict[int, int]:
    # Matches firmware runStandPose():
    #   R1=135, R2=45, L1=45, L2=135 (hips)
    #   R4=0,   R3=180, L3=0,  L4=180 (knees)
    return {0: 135, 1: 45, 2: 45, 3: 135, 4: 0, 5: 180, 6: 0, 7: 180}


class _Builder:
    """Stateful helper: tracks cumulative servo state and emits Keyframes.

    The firmware sets servos and then calls delayWithFace(ms).  _Builder
    mirrors that: call set(**named_angles) any number of times, then
    delay(ms) to snapshot the current state as a Keyframe.
    """

    def __init__(self, initial: dict[int, int] | None = None) -> None:
        self._s: dict[int, int] = dict(initial) if initial else _all_90()
        self._frames: list[Keyframe] = []

    def set(self, **kwargs: int) -> "_Builder":
        for name, deg in kwargs.items():
            self._s[_NAME_TO_IDX[name]] = deg
        return self

    def delay(self, ms: int) -> "_Builder":
        self._frames.append(Keyframe(dict(self._s), ms))
        return self

    def stand(self, hold_ms: int = 0) -> "_Builder":
        self._s.update(_stand_state())
        if hold_ms:
            self._frames.append(Keyframe(dict(self._s), hold_ms))
        return self

    def build(self) -> list[Keyframe]:
        return list(self._frames)


# ---------------------------------------------------------------------------
# Sequence definitions — translated 1-to-1 from movement-sequences.h
# ---------------------------------------------------------------------------

def _seq_rest() -> list[Keyframe]:
    return [Keyframe(_all_90(), 1000)]


def _seq_stand() -> list[Keyframe]:
    return [Keyframe(_stand_state(), 1000)]


def _seq_wave(cycles: int = 4) -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(R4=80, L3=180, L2=60, R1=100).delay(200)
    b.delay(300)  # L3=180 hold (no change from firmware)
    for _ in range(cycles):
        b.set(L3=180).delay(300)
        b.set(L3=100).delay(300)
    b.stand(500)
    return b.build()


def _seq_dance(cycles: int = 5) -> list[Keyframe]:
    b = _Builder()
    # hips stay at 90 (already in initial state); set knees
    b.set(R4=160, R3=160, L3=10, L4=10).delay(300)
    for _ in range(cycles):
        b.set(R4=115, R3=115, L3=10, L4=10).delay(300)
        b.set(R4=160, R3=160, L3=65, L4=65).delay(300)
    b.stand(500)
    return b.build()


def _seq_swim(cycles: int = 4) -> list[Keyframe]:
    b = _Builder()  # starts all-90
    for _ in range(cycles):
        b.set(R1=135, R2=45, L1=45, L2=135).delay(400)
        b.set(R1=90, R2=90, L1=90, L2=90).delay(400)
    b.stand(500)
    return b.build()


def _seq_point() -> list[Keyframe]:
    b = _Builder()
    b.set(L2=60, R1=135, R2=100, L4=180, L1=25, L3=145, R4=80, R3=170).delay(2000)
    b.stand(500)
    return b.build()


def _seq_pushup(reps: int = 4) -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(L1=0, R1=180, L3=90, R3=90).delay(500)
    for _ in range(reps):
        b.set(L3=0, R3=180).delay(600)
        b.set(L3=90, R3=90).delay(500)
    b.stand(500)
    return b.build()


def _seq_bow() -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(L1=0, R1=180, L3=0, R3=180, L2=180, R2=0, R4=0, L4=180).delay(600)
    b.set(L3=90, R3=90).delay(3000)
    b.stand(500)
    return b.build()


def _seq_cute(cycles: int = 5) -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(L2=160, R2=20, R4=180, L4=0, L1=0, R1=180, L3=180, R3=0).delay(200)
    for _ in range(cycles):
        b.set(R4=180, L4=45).delay(300)
        b.set(R4=135, L4=0).delay(300)
    b.stand(500)
    return b.build()


def _seq_freaky(cycles: int = 3) -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(L1=0, R1=180, L2=180, R2=0, R4=90, R3=0).delay(200)
    for _ in range(cycles):
        b.set(R3=25).delay(400)
        b.set(R3=0).delay(400)
    b.stand(500)
    return b.build()


def _seq_worm(cycles: int = 5) -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(R1=180, R2=0, L1=0, L2=180, R4=90, R3=90, L3=90, L4=90).delay(200)
    for _ in range(cycles):
        b.set(R3=45, L3=135, R4=45, L4=135).delay(300)
        b.set(R3=135, L3=45, R4=135, L4=45).delay(300)
    b.stand(500)
    return b.build()


def _seq_shake(cycles: int = 5) -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(R1=135, L1=45, L3=90, R3=90, L2=90, R2=90).delay(200)
    for _ in range(cycles):
        b.set(R4=45, L4=135).delay(300)
        b.set(R4=0, L4=180).delay(300)
    b.stand(500)
    return b.build()


def _seq_shrug() -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(R3=90, R4=90, L3=90, L4=90).delay(1000)
    b.set(R3=0, R4=180, L3=180, L4=0).delay(1500)
    b.stand(500)
    return b.build()


def _seq_dead() -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(R3=90, R4=90, L3=90, L4=90).delay(2000)
    return b.build()


def _seq_crab(cycles: int = 5) -> list[Keyframe]:
    b = _Builder()
    b.stand(200)
    b.set(R1=90, R2=90, L1=90, L2=90, R4=0, R3=180, L3=45, L4=135).delay(200)
    for _ in range(cycles):
        b.set(R4=45, R3=135, L3=0, L4=180).delay(300)
        b.set(R4=0, R3=180, L3=45, L4=135).delay(300)
    b.stand(500)
    return b.build()


def _seq_walk(cycles: int = 3, frame_delay_ms: int = 200) -> list[Keyframe]:
    # pressingCheck("forward", frameDelay) → fixed delay per gait step
    b = _Builder()
    b.set(R3=135, L3=45, R2=100, L1=25).delay(frame_delay_ms)
    for _ in range(cycles):
        b.set(R3=135, L3=0).delay(frame_delay_ms)
        b.set(L4=135, L2=90, R4=0, R1=180).delay(frame_delay_ms)
        b.set(R2=45, L1=90).delay(frame_delay_ms)
        b.set(R4=45, L4=180).delay(frame_delay_ms)
        b.set(R3=180, L3=45, R2=90, L1=0).delay(frame_delay_ms)
        b.set(L2=135, R1=90).delay(frame_delay_ms)
    b.stand(500)
    return b.build()


def _seq_backward(cycles: int = 3, frame_delay_ms: int = 200) -> list[Keyframe]:
    b = _Builder()
    b.delay(frame_delay_ms)  # initial pressingCheck hold
    for _ in range(cycles):
        b.set(R3=135, L3=0).delay(frame_delay_ms)
        b.set(L4=135, L2=135, R4=0, R1=90).delay(frame_delay_ms)
        b.set(R2=90, L1=0).delay(frame_delay_ms)
        b.set(R4=45, L4=180).delay(frame_delay_ms)
        b.set(R3=180, L3=45, R2=45, L1=90).delay(frame_delay_ms)
        b.set(L2=90, R1=180).delay(frame_delay_ms)
    b.stand(500)
    return b.build()


def _seq_turn_left(cycles: int = 3, frame_delay_ms: int = 200) -> list[Keyframe]:
    b = _Builder()
    for _ in range(cycles):
        # legset 1 (R1, L2)
        b.set(R3=135, L4=135).delay(frame_delay_ms)
        b.set(R1=180, L2=180).delay(frame_delay_ms)
        b.set(R3=180, L4=180).delay(frame_delay_ms)
        b.set(R1=135, L2=135).delay(frame_delay_ms)
        # legset 2 (R2, L1)
        b.set(R4=45, L3=45).delay(frame_delay_ms)
        b.set(R2=90, L1=90).delay(frame_delay_ms)
        b.set(R4=0, L3=0).delay(frame_delay_ms)
        b.set(R2=45, L1=45).delay(frame_delay_ms)
    b.stand(500)
    return b.build()


def _seq_turn_right(cycles: int = 3, frame_delay_ms: int = 200) -> list[Keyframe]:
    b = _Builder()
    for _ in range(cycles):
        # legset 2 (R2, L1)
        b.set(R4=45, L3=45).delay(frame_delay_ms)
        b.set(R2=0, L1=0).delay(frame_delay_ms)
        b.set(R4=0, L3=0).delay(frame_delay_ms)
        b.set(R2=45, L1=45).delay(frame_delay_ms)
        # legset 1 (R1, L2)
        b.set(R3=135, L4=135).delay(frame_delay_ms)
        b.set(R1=90, L2=90).delay(frame_delay_ms)
        b.set(R3=180, L4=180).delay(frame_delay_ms)
        b.set(R1=135, L2=135).delay(frame_delay_ms)
    b.stand(500)
    return b.build()


def _build_registry(cycles: int) -> dict[str, list[Keyframe]]:
    return {
        "rest":       _seq_rest(),
        "stand":      _seq_stand(),
        "wave":       _seq_wave(),
        "dance":      _seq_dance(),
        "swim":       _seq_swim(),
        "point":      _seq_point(),
        "pushup":     _seq_pushup(),
        "bow":        _seq_bow(),
        "cute":       _seq_cute(),
        "freaky":     _seq_freaky(),
        "worm":       _seq_worm(),
        "shake":      _seq_shake(),
        "shrug":      _seq_shrug(),
        "dead":       _seq_dead(),
        "crab":       _seq_crab(),
        "walk":       _seq_walk(cycles),
        "backward":   _seq_backward(cycles),
        "turn_left":  _seq_turn_left(cycles),
        "turn_right": _seq_turn_right(cycles),
    }


# ---------------------------------------------------------------------------
# Action conversion
# ---------------------------------------------------------------------------

def _keyframe_to_action(
    kf: Keyframe,
    joint_names: list[str],
    default_angles: dict[str, float],
    action_scale: float,
) -> torch.Tensor:
    """Convert a firmware keyframe to a MuJoCo action tensor.

    target_rad = servo_degrees * pi / 180
    action[i]  = (target_rad - default_angle[joint]) / action_scale
    """
    action = torch.zeros(len(joint_names))
    for i, joint in enumerate(joint_names):
        servo_idx = _JOINT_TO_SERVO[joint]
        target_rad = kf.angles[servo_idx] * math.pi / 180.0
        action[i] = (target_rad - default_angles[joint]) / action_scale
    return action


# ---------------------------------------------------------------------------
# Sequence player (policy-compatible callable)
# ---------------------------------------------------------------------------

class SequencePlayer:
    """Steps through a list of Keyframes, cycling indefinitely.

    Compatible with ViserPlayViewer / NativeMujocoViewer — call it like a
    policy: player(obs) → action tensor of shape (1, n_joints).
    """

    def __init__(
        self,
        keyframes: list[Keyframe],
        joint_names: list[str],
        default_angles: dict[str, float],
        action_scale: float,
        control_dt_s: float,
        device: str,
    ) -> None:
        self._keyframes = keyframes
        self._joint_names = joint_names
        self._default_angles = default_angles
        self._action_scale = action_scale
        self._control_dt_s = control_dt_s
        self._device = device
        self._kf_idx = 0
        self._steps_remaining = 0

    def __call__(self, _obs) -> torch.Tensor:
        if self._steps_remaining <= 0:
            self._kf_idx = (self._kf_idx + 1) % len(self._keyframes)
            kf = self._keyframes[self._kf_idx]
            self._steps_remaining = max(
                1, round(kf.duration_ms / 1000.0 / self._control_dt_s)
            )
        self._steps_remaining -= 1
        action = _keyframe_to_action(
            self._keyframes[self._kf_idx],
            self._joint_names,
            self._default_angles,
            self._action_scale,
        )
        return action.unsqueeze(0).to(self._device)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Replay firmware gait sequences in MuJoCo simulation."
    )
    p.add_argument(
        "--sequence", "-s",
        metavar="NAME",
        help="Sequence to replay (default: all in order).",
    )
    p.add_argument(
        "--cycles", type=int, default=3,
        help="Loop count for walk/turn sequences (default: 3).",
    )
    p.add_argument(
        "--viewer", choices=("viser", "native"), default="viser",
    )
    p.add_argument(
        "--list", action="store_true",
        help="Print available sequence names and exit.",
    )
    return p.parse_args()


def main() -> None:
    args = _parse_args()
    registry = _build_registry(args.cycles)

    if args.list:
        for name in registry:
            print(name)
        return

    if args.sequence is not None:
        if args.sequence not in registry:
            raise SystemExit(
                f"Unknown sequence '{args.sequence}'. "
                f"Available: {', '.join(registry)}"
            )
        keyframes = registry[args.sequence]
    else:
        keyframes = [kf for seq in registry.values() for kf in seq]

    orig_terminate = C.TERMINATE_ON_FALL
    orig_num_envs = C.NUM_ENVS
    C.TERMINATE_ON_FALL = False
    C.NUM_ENVS = 1

    try:
        env_cfg = sesame_flat_env_cfg(play=True)
        device = "cuda"
        env = ManagerBasedRlEnv(cfg=env_cfg, device=device)

        joint_names = env.action_manager.get_term("joint_pos").target_names
        print(f"[sequences] joint order: {joint_names}")
        print(
            f"[sequences] playing: {args.sequence or 'all'} "
            f"({len(keyframes)} keyframes)"
        )

        control_dt_s = env_cfg.decimation * env_cfg.sim.mujoco.timestep
        player = SequencePlayer(
            keyframes=keyframes,
            joint_names=joint_names,
            default_angles=C.DEFAULT_JOINT_ANGLES,
            action_scale=C.ACTION_SCALE,
            control_dt_s=control_dt_s,
            device=device,
        )

        wrapped = RslRlVecEnvWrapper(env, clip_actions=None)
        viewer_cls = ViserPlayViewer if args.viewer == "viser" else NativeMujocoViewer
        viewer_cls(wrapped, player).run()
        wrapped.close()
    finally:
        C.TERMINATE_ON_FALL = orig_terminate
        C.NUM_ENVS = orig_num_envs


if __name__ == "__main__":
    main()
