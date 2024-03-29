import json
import pathlib
from typing import Any, Callable

from libcasm.monte import ValueMap

from ._clexmonte_state import (
    MonteCarloState,
)
from ._RunData import (
    RunData,
    RunDataOutputParams,
)


class IncrementalConditionsStateGenerator:
    """A `StateGenerator` - Generates the initial state for a series of Monte Carlo \
    runs as a linear path in thermodynamic conditions space

    """

    def __init__(
        self,
        output_params: RunDataOutputParams,
        initial_conditions: ValueMap,
        conditions_increment: ValueMap,
        n_states: int,
        config_generator: Any,
        dependent_runs: bool,
        modifiers: list[Callable[[MonteCarloState], None]] = [],
    ):
        """
        .. rubric :: Constructor

        Parameters
        ----------
        output_params: RunDataOutputParams
            Controls saving and writing completed run data. At least the final
            state must be saved to enable restarts and dependent runs.
        initial_conditions: libcasm.monte.ValueMap
            The thermodynamic conditions for the initial run in the series
        conditions_increment: libcasm.monte.ValueMap
            The change in thermodynamic conditions for each subsequent run
        n_states: int
            The number of states to generate / number of Monte Carlo runs in the series
        config_generator: Callable[[libcasm.monte.ValueMap, \
        list[RunData]] MonteCarloConfiguration]
            A functor which returns a configuration based on thermodynamic conditions
            for the next Monte Carlo run and the RunData of the previously completed
            runs. Usually a :class:`libcasm.clexmonte.FixedConfigGenerator` instance.
        dependent_runs: bool = True
            If True, states are constructed using the final configuration of the
            previous run. Otherwise, states are constructed using the configuration
            generated by `config_generator`. The first state is always constructed
            using `config_generator`.
        modifiers: list[Callable[[MonteCarloState] None]] = []
            A list of one or more functions that modify the generated state before it
            is returned. This can be used to apply custom constraints.
        """
        self._completed_runs = []
        self._output_params = output_params
        self._config_generator = config_generator
        self._initial_conditions = initial_conditions
        self._conditions_increment = conditions_increment
        self._n_states = n_states
        self._dependent_runs = dependent_runs
        self._modifiers = modifiers

        if self._initial_conditions.is_mismatched(self._conditions_increment):
            raise Exception(
                "Error constructing IncrementalConditionsStateGenerator: "
                "Mismatch between initial conditions and conditions increment."
            )

    @property
    def is_complete(self) -> bool:
        """Check if all requested runs have been completed"""
        return len(self._completed_runs) == self._n_states

    @property
    def next_state(self) -> MonteCarloState:
        """Construct and return the next state

        Returns
        -------
        state: libcasm.clexmonte.MonteCarloState
            The next state in the series
        """
        if (
            self._dependent_runs
            and self.n_completed_runs
            and self.completed_runs[-1].final_state is None
        ):
            raise Exception(
                "Error in IncrementalConditionsStateGenerator: "
                "when dependent_runs==true, must save the final state of the last "
                "completed run"
            )

        # Make conditions
        conditions = self._initial_conditions.make_incremented_values(
            self._conditions_increment,
            self.n_completed_runs,
        )

        # Make configuration
        if self._dependent_runs and self.n_completed_runs:
            configuration = self.completed_runs[-1].final_state.configuration
        else:
            configuration = self._config_generator(conditions, self.completed_runs)

        # Make state
        state = MonteCarloState(configuration=configuration, conditions=conditions)

        # Apply custom modifiers
        for f in self._modifiers:
            f(state)

        return state

    def append(self, run_data: RunData):
        """Append a completed run

        Notes
        -----
        - Initial and final state data may be erased depending on
          the settings provided constructor parameter `output_params`
        """
        if len(self._completed_runs):
            if not self._output_params.do_save_all_final_states:
                self._completed_runs[-1].final_state = None
        self._completed_runs.append(run_data)
        if not self._output_params.do_save_all_initial_states:
            self._completed_runs[-1].initial_state = None
        if (
            not self._output_params.do_save_last_final_state
            and not self._output_params.do_save_all_final_states
        ):
            self._completed_runs[-1].final_state = None

    @property
    def completed_runs(self) -> list[RunData]:
        """The list of completed runs"""
        return self._completed_runs

    @property
    def n_completed_runs(self) -> int:
        """Return the number of completed runs"""
        return len(self._completed_runs)

    def read_completed_runs(self):
        """Read completed runs data

        Notes
        -----
        - Reads from ``output_params.output_dir / "completed_runs.json"``.
        - Skips if file does not exist or `output_params.output_dir` is None.
        """
        if self._output_params.output_dir is None:
            return

        output_dir = pathlib.Path(self._output_params.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        completed_runs_path = output_dir / "completed_runs.json"
        if not completed_runs_path.exists():
            return
        with open(completed_runs_path, "r") as f:
            data = json.load(f)
            try:
                self._completed_runs = [RunData.from_dict(x) for x in data]
            except Exception as e:
                print("what:", e)
                raise Exception(
                    f"Error in IncrementalConditionsStateGenerator: "
                    f"failed to read {completed_runs_path}"
                )

    def write_completed_runs(self):
        """Write completed runs data

        Notes
        -----
        - Writes to ``output_params.output_dir / "completed_runs.json"``.
        """
        if self._output_params.output_dir is None:
            return

        output_dir = pathlib.Path(self._output_params.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        completed_runs_path = output_dir / "completed_runs.json"
        with open(completed_runs_path, "w") as f:
            data = [
                x.to_dict(
                    do_write_initial_states=self._output_params.do_write_initial_states,
                    do_write_final_states=self._output_params.do_write_final_states,
                )
                for x in self._completed_runs
            ]
            f.write(json.dumps(data))
