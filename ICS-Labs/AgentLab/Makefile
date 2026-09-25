.PHONY: help all sync format compile check run shell eval grade eval-all eval-read eval-list-files eval-edit-file eval-bash eval-memory-recall eval-memory-update eval-redaction eval-ephemeral eval-review clean

PYTHON := uv run python
PROVIDER ?= openrouter
MODEL ?=
SEED ?= 42
TEMPERATURE ?= 0
PROMPT ?= please read a.txt
SCENARIO ?= evals/read_file_efficiency.json

all: check

help:
	@echo "ICS Agent Lab commands"
	@echo ""
	@echo "Setup:"
	@echo "  make sync                         Install/update uv environment"
	@echo ""
	@echo "Quality:"
	@echo "  make format                       Run black and isort"
	@echo "  make compile                      Compile Python files"
	@echo "  make check                        Format and compile"
	@echo ""
	@echo "Run:"
	@echo "  make run PROMPT='please read a.txt'"
	@echo "  make shell                        Interactive OpenRouter shell"
	@echo ""
	@echo "Evaluation:"
	@echo "  make grade                        Run all evals and report score"
	@echo "  make eval-all                     Alias for make grade"
	@echo "  make eval SCENARIO=evals/read_file_efficiency.json"
	@echo "  make eval-read                    Built-in read_file efficiency eval"
	@echo "  make eval-list-files              Built-in list_files eval"
	@echo "  make eval-edit-file               Built-in edit_file eval"
	@echo "  make eval-bash                    Built-in bash eval"
	@echo "  make eval-memory-recall           Memory compaction recall eval"
	@echo "  make eval-memory-update           Memory update/temporal eval"
	@echo "  make eval-redaction               Data redaction eval"
	@echo "  make eval-ephemeral               Ephemeral dispatch eval"
	@echo "  make eval-review                  Patch review eval"
	@echo ""
	@echo "Cleanup:"
	@echo "  make clean                        Remove generated traces/caches"

sync:
	uv sync

format:
	uv run black ics_agent_lab lab_eval assignments
	uv run isort ics_agent_lab lab_eval assignments

compile:
	$(PYTHON) -m compileall ics_agent_lab lab_eval assignments

check: format compile

run:
	$(PYTHON) main.py --provider $(PROVIDER) $(if $(MODEL),--model $(MODEL),) --temperature $(TEMPERATURE) --seed $(SEED) "$(PROMPT)"

shell:
	$(PYTHON) main.py --provider $(PROVIDER) $(if $(MODEL),--model $(MODEL),) --temperature $(TEMPERATURE) --seed $(SEED)

eval:
	$(PYTHON) -m lab_eval "$(SCENARIO)" --provider $(PROVIDER) $(if $(MODEL),--model $(MODEL),) --temperature $(TEMPERATURE) --seed $(SEED)

grade:
	$(PYTHON) -m lab_eval.suite --provider $(PROVIDER) $(if $(MODEL),--model $(MODEL),) --temperature $(TEMPERATURE) --seed $(SEED)

eval-all: grade

eval-read:
	$(PYTHON) -m lab_eval evals/read_file_efficiency.json

eval-list-files:
	$(PYTHON) -m lab_eval evals/list_files_nested.json

eval-edit-file:
	$(PYTHON) -m lab_eval evals/edit_file_replace.json

eval-bash:
	$(PYTHON) -m lab_eval evals/bash_workspace.json

eval-memory-recall:
	$(PYTHON) -m lab_eval evals/memory_persistent_recall.json

eval-memory-update:
	$(PYTHON) -m lab_eval evals/memory_persistent_update.json

eval-redaction:
	$(PYTHON) -m lab_eval assignments/data_redaction/eval.json

eval-ephemeral:
	$(PYTHON) -m lab_eval assignments/ephemeral_dispatch/eval.json

eval-review:
	$(PYTHON) -m lab_eval assignments/patch_review/eval.json

clean:
	find . -type d -name __pycache__ -prune -exec rm -rf {} +
	rm -rf traces
