import matplotlib.pyplot as plt
import numpy as np


def explicit_euler_cos(
    timestep: float, target_time: float
) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    n_steps = int(np.rint(target_time / timestep))  # Avoids truncation

    # Initial Conditions
    xs = np.zeros((n_steps + 1,))
    vs = np.zeros((n_steps + 1,))
    ts = np.zeros((n_steps + 1,))
    epss = np.zeros((n_steps + 1,))

    xs[0] = 1.0
    vs[0] = 0.0

    for i in range(1, n_steps + 1):
        xs[i] = xs[i - 1] + timestep * vs[i - 1]
        vs[i] = vs[i - 1] - timestep * xs[i - 1]
        ts[i] = ts[i - 1] + timestep
        epss[i] = abs(xs[i] - np.cos(ts[i]))

    return xs, vs, ts, epss


def main() -> None:
    h1 = 0.1
    h2 = 0.5 * h1
    h3 = 0.5 * h2

    for t in [0.1, 0.2, 0.3]:
        print(f"{t=:.1f}")
        sim_h1 = explicit_euler_cos(h1, t)
        sim_h2 = explicit_euler_cos(h2, t)
        sim_h3 = explicit_euler_cos(h3, t)
        print(f"h1: {abs(sim_h1[0][-1] - np.cos(t)):.8f}")
        print(f"h2: {abs(sim_h2[0][-1] - np.cos(t)):.8f}")
        print(f"h3: {abs(sim_h3[0][-1] - np.cos(t)):.8f}")

    xs1, _, ts1, eps1 = explicit_euler_cos(h1, 0.3)
    xs2, _, ts2, eps2 = explicit_euler_cos(h2, 0.3)
    xs3, _, ts3, eps3 = explicit_euler_cos(h3, 0.3)

    plt.plot(ts1, xs1, label=r"$x^{0.1}$")
    plt.plot(ts2, xs2, label=r"$x^{0.2}$")
    plt.plot(ts3, xs3, label=r"$x^{0.3}$")
    plt.plot(ts3, np.cos(ts3), label="Exact Solution")
    plt.xlabel(r"$t$")
    plt.ylabel(r"$x(t)$")
    plt.legend()
    plt.show()

    plt.plot(ts1, eps1, label=r"$\varepsilon^{0.1}$")
    plt.plot(ts2, eps2, label=r"$\varepsilon^{0.05}$")
    plt.plot(ts3, eps3, label=r"$\varepsilon^{0.025}$")
    plt.xlabel(r"$t$")
    plt.ylabel(r"$\varepsilon(t)$")
    plt.legend()
    plt.show()


if __name__ == "__main__":
    main()
