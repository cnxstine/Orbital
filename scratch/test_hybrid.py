import math

# Constants
pi = math.pi

def psi_2s(r):
    # psi = 1/(4*sqrt(2*pi)) * (2 - r) * e^(-r/2)
    return (1.0 / (4.0 * math.sqrt(2.0 * pi))) * (2.0 - r) * math.exp(-r / 2.0)

def psi_2p(x, y, z):
    r = math.sqrt(x*x + y*y + z*z)
    # radialPart = 1/(4*sqrt(2*pi)) * e^(-r/2)
    radial_part = (1.0 / (4.0 * math.sqrt(2.0 * pi))) * math.exp(-r / 2.0)
    return radial_part * x, radial_part * y, radial_part * z

# sp hybridization
def eval_sp(idx, x, y, z):
    r = math.sqrt(x*x + y*y + z*z)
    s = psi_2s(r)
    _, _, pz = psi_2p(x, y, z)
    c2s = 1.0 / math.sqrt(2.0)
    c2pz = 1.0 / math.sqrt(2.0) if idx == 0 else -1.0 / math.sqrt(2.0)
    psi = c2s * s + c2pz * pz
    return psi**2

# sp2 hybridization
def eval_sp2(idx, x, y, z):
    r = math.sqrt(x*x + y*y + z*z)
    s = psi_2s(r)
    px, py, _ = psi_2p(x, y, z)
    
    c2s = 1.0 / math.sqrt(3.0)
    if idx == 0:
        c2px = math.sqrt(2.0 / 3.0)
        c2py = 0.0
    elif idx == 1:
        c2px = -1.0 / math.sqrt(6.0)
        c2py = 1.0 / math.sqrt(2.0)
    else:
        c2px = -1.0 / math.sqrt(6.0)
        c2py = -1.0 / math.sqrt(2.0)
        
    psi = c2s * s + c2px * px + c2py * py
    return psi**2

# sp3 hybridization
def eval_sp3(idx, x, y, z):
    r = math.sqrt(x*x + y*y + z*z)
    s = psi_2s(r)
    px, py, pz = psi_2p(x, y, z)
    
    c2s = 0.5
    if idx == 0:
        c2px, c2py, c2pz = 0.5, 0.5, 0.5
    elif idx == 1:
        c2px, c2py, c2pz = 0.5, -0.5, -0.5
    elif idx == 2:
        c2px, c2py, c2pz = -0.5, 0.5, -0.5
    else:
        c2px, c2py, c2pz = -0.5, -0.5, 0.5
        
    psi = c2s * s + c2px * px + c2py * py + c2pz * pz
    return psi**2

print("=== Testing sp at different distances ===")
for dist in [1.0, 1.5, 2.0, 3.0]:
    d_pos = eval_sp(0, 0, 0, dist)
    d_neg = eval_sp(0, 0, 0, -dist)
    ratio = d_pos / d_neg if d_neg > 0 else float('inf')
    print(f"Dist: {dist:.1f} Bohr | posZ: {d_pos:.8f} | negZ: {d_neg:.8f} | ratio: {ratio:.2f}")

print("\n=== Testing sp2 at different distances ===")
for dist in [1.0, 1.5, 2.0, 3.0]:
    # Peak direction for sp2_0 is (1, 0, 0), secondary is (-0.5, sqrt(3)/2, 0)
    v0 = (dist, 0.0, 0.0)
    v1 = (-0.5 * dist, math.sqrt(3.0)/2.0 * dist, 0.0)
    d0 = eval_sp2(0, *v0)
    d1 = eval_sp2(0, *v1)
    ratio = d0 / d1 if d1 > 0 else float('inf')
    print(f"Dist: {dist:.1f} Bohr | peak: {d0:.8f} | secondary: {d1:.8f} | ratio: {ratio:.2f}")

print("\n=== Testing sp3 at different distances ===")
for dist in [1.0, 1.5, 2.0, 3.0]:
    # Peak direction for sp3_0 is (1/sqrt(3), 1/sqrt(3), 1/sqrt(3))
    # Secondary is (1/sqrt(3), -1/sqrt(3), -1/sqrt(3))
    val = 1.0 / math.sqrt(3.0)
    v0 = (val * dist, val * dist, val * dist)
    v1 = (val * dist, -val * dist, -val * dist)
    d0 = eval_sp3(0, *v0)
    d1 = eval_sp3(0, *v1)
    ratio = d0 / d1 if d1 > 0 else float('inf')
    print(f"Dist: {dist:.1f} Bohr | peak: {d0:.8f} | secondary: {d1:.8f} | ratio: {ratio:.2f}")
