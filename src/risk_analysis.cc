/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file risk_analysis.cc
/// Implementation of risk analysis handler.

#include "risk_analysis.h"

#include "bdd.h"
#include "fault_tree.h"
#include "logger.h"
#include "mocus.h"
#include "random.h"
#include "zbdd.h"

namespace scram {
namespace core {

RiskAnalysis::RiskAnalysis(std::shared_ptr<const mef::Model> model,
                           const Settings& settings)
    : Analysis(settings),
      model_(std::move(model)) {}

void RiskAnalysis::Analyze() noexcept {
  // Set the seed for the pseudo-random number generator if given explicitly.
  // Otherwise it defaults to the implementation dependent value.
  if (Analysis::settings().seed() >= 0)
    Random::seed(Analysis::settings().seed());

  for (const mef::FaultTreePtr& ft : model_->fault_trees()) {
    for (const mef::Gate* target : ft->top_events()) {
      LOG(INFO) << "Running analysis: " << target->id();
      RunAnalysis(target->id(), *target);
      LOG(INFO) << "Finished analysis: " << target->id();
    }
  }
}

void RiskAnalysis::RunAnalysis(const std::string& name,
                               const mef::Gate& target) noexcept {
  const std::string& algorithm = Analysis::settings().algorithm();
  if (algorithm == "bdd") {
    RunAnalysis<Bdd>(name, target);
  } else if (algorithm == "zbdd") {
    RunAnalysis<Zbdd>(name, target);
  } else {
    assert(algorithm == "mocus");
    RunAnalysis<Mocus>(name, target);
  }
}

template <class Algorithm>
void RiskAnalysis::RunAnalysis(const std::string& name,
                               const mef::Gate& target) noexcept {
  auto fta =
      std::make_unique<FaultTreeAnalyzer<Algorithm>>(target,
                                                     Analysis::settings());
  fta->Analyze();
  if (Analysis::settings().probability_analysis()) {
    if (Analysis::settings().approximation() == "no") {
      RunAnalysis<Algorithm, Bdd>(name, fta.get());
    } else if (Analysis::settings().approximation() == "rare-event") {
      RunAnalysis<Algorithm, RareEventCalculator>(name, fta.get());
    } else {
      assert(Analysis::settings().approximation() == "mcub");
      RunAnalysis<Algorithm, McubCalculator>(name, fta.get());
    }
  }
  fault_tree_analyses_.emplace(name, std::move(fta));
}

template <class Algorithm, class Calculator>
void RiskAnalysis::RunAnalysis(const std::string& name,
                               FaultTreeAnalyzer<Algorithm>* fta) noexcept {
  auto pa = std::make_unique<ProbabilityAnalyzer<Calculator>>(fta);
  pa->Analyze();
  if (Analysis::settings().importance_analysis()) {
    auto ia = std::make_unique<ImportanceAnalyzer<Calculator>>(pa.get());
    ia->Analyze();
    importance_analyses_.emplace(name, std::move(ia));
  }
  if (Analysis::settings().uncertainty_analysis()) {
    auto ua = std::make_unique<UncertaintyAnalyzer<Calculator>>(pa.get());
    ua->Analyze();
    uncertainty_analyses_.emplace(name, std::move(ua));
  }
  probability_analyses_.emplace(name, std::move(pa));
}

}  // namespace core
}  // namespace scram
