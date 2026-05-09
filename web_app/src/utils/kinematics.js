import { L1, L2, L3 } from '../constants/robotConfig'

const DEG2RAD = (d) => (d * Math.PI) / 180
const RAD2DEG = (r) => (r * 180) / Math.PI
const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v))

/**
 * Forward kinematics — returns {x, y, z} foot position in leg-local frame
 */
export function forwardKinematics(coxaDeg, femurDeg, tibiaDeg) {
  const t1 = DEG2RAD(coxaDeg)
  const t2 = DEG2RAD(femurDeg)
  const t3 = DEG2RAD(tibiaDeg)
  const reach = L1 + L2 * Math.cos(t2) + L3 * Math.cos(t2 + t3)
  return {
    x: reach * Math.cos(t1),
    y: reach * Math.sin(t1),
    z: L2 * Math.sin(t2) + L3 * Math.sin(t2 + t3),
  }
}

/**
 * Inverse kinematics — tibia always vertical (perpendicular to ground)
 * femurDeg: driven by Z only  (sin(t2) = (z - L3) / L2)
 * tibiaDeg: 90° - femurDeg    (keeps tibia vertical, exactly)
 * L2 constraint always satisfied.
 */
export function inverseKinematics(x, y, z) {
  const coxaDeg = RAD2DEG(Math.atan2(y, x))
  const sin_t2 = (z - L3) / L2
  if (sin_t2 < -1 || sin_t2 > 1) {
    return { coxaDeg: 0, femurDeg: 0, tibiaDeg: 0, valid: false }
  }
  const femurDeg = RAD2DEG(Math.asin(sin_t2))
  const tibiaDeg = 90 - femurDeg
  return { coxaDeg, femurDeg, tibiaDeg, valid: true }
}

/**
 * Convert body-frame foot target → world foot position
 * mountDeg: leg attachment angle on body
 */
export function bodyToLegFrame(bx, by, mountDeg) {
  const rad = DEG2RAD(mountDeg)
  return {
    lx:  bx * Math.cos(rad) + by * Math.sin(rad),
    ly: -bx * Math.sin(rad) + by * Math.cos(rad),
  }
}
