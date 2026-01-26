# experiments/integrators.py
import matplotlib.pyplot as plt
import numpy as np


def explicit_euler_cos(
    timestep: float, target_time: float, x0=1.0, v0=0.0
) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    n_steps = int(np.rint(target_time / timestep))  # Avoids truncation

    # Initial Conditions
    xs = np.zeros((n_steps + 1,))
    vs = np.zeros((n_steps + 1,))
    ts = np.zeros((n_steps + 1,))
    epss = np.zeros((n_steps + 1,))

    xs[0] = x0
    vs[0] = v0

    for i in range(1, n_steps + 1):
        xs[i] = xs[i - 1] + timestep * vs[i - 1]
        vs[i] = vs[i - 1] - timestep * xs[i - 1]
        ts[i] = ts[i - 1] + timestep
        epss[i] = abs(xs[i] - np.cos(ts[i]))

    return xs, vs, ts, epss


def error_curve() -> None:
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

    _, (ax1, ax2) = plt.subplots(2, 1, sharex=True)

    ax1.plot(ts1, xs1, label=r"$x^{0.1}$")
    ax1.plot(ts2, xs2, label=r"$x^{0.05}$")
    ax1.plot(ts3, xs3, label=r"$x^{0.025}$")
    ax1.plot(ts3, np.cos(ts3), label="Exact Solution")
    ax1.set_ylabel(r"$x(t)$")
    ax1.legend()

    ax2.plot(ts1, eps1, label=r"$\varepsilon^{0.1}$")
    ax2.plot(ts2, eps2, label=r"$\varepsilon^{0.05}$")
    ax2.plot(ts3, eps3, label=r"$\varepsilon^{0.025}$")
    ax2.set_xlabel(r"$t$")
    ax2.set_ylabel(r"$\varepsilon(t)$")
    ax2.legend()

    plt.tight_layout()
    plt.savefig("explicit_euler_error_curve.png", dpi=300)
    plt.show()


def phase_diagram_multiple_ics() -> None:
    h = 0.1
    T = 20.0

    initial_conditions = [
        (1.0, 0.0),
        (0.5, 2.0),
    ]

    for x0, v0 in initial_conditions:
        xs, vs, _, _ = explicit_euler_cos(h, T, x0=x0, v0=v0)
        plt.plot(xs, vs, label=rf"$x_0={x0},\,\dot x_0={v0}$")

    theta = np.linspace(0.0, 2.0 * np.pi, 512)
    label_shown = False
    for r in [np.sqrt(x0**2 + v0**2) for x0, v0 in initial_conditions]:
        label = None
        if not label_shown:
            label = "Exact circle"
            label_shown = True

        plt.plot(r * np.cos(theta), -r * np.sin(theta), "k--", alpha=0.3, label=label)

    plt.xlabel(r"$x$")
    plt.ylabel(r"$\dot{x}$")
    plt.axis("equal")
    plt.legend()

    plt.tight_layout()
    plt.savefig("explicit_euler_phase_diagram.png", dpi=300)
    plt.show()


def main() -> None:
    error_curve()
    phase_diagram_multiple_ics()


if __name__ == "__main__":
    main()
