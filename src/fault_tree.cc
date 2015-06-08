/// @file fault_tree.cc
/// Implementation of fault tree analysis.
#include "fault_tree.h"

#include <algorithm>
#include <map>

#include <boost/pointer_cast.hpp>

#include "error.h"

namespace scram {

FaultTree::FaultTree(std::string name) : name_(name), num_basic_events_(0) {}

void FaultTree::AddGate(const GatePtr& gate) {
  if (gates_.count(gate->id())) {
    throw ValidationError("Trying to redefine gate " + gate->name() + ".");
  }
  gates_.insert(std::make_pair(gate->id(), gate));
}

void FaultTree::Validate() {
  // Detects the top event. Currently only one top event is allowed.
  /// @todo Add support for multiple top event.
  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = gates_.begin(); it != gates_.end(); ++it) {
    if (!it->second->IsOrphan()) {
      inter_events_.insert(*it);
    } else {
      if (top_event_) {
        throw ValidationError("Multiple top events are detected: " +
                              top_event_->name() + " and " +
                              it->second->name() + " in " + name_ +
                              " fault tree.");
      }
      top_event_ = it->second;
    }
  }

  // The gate structure must be checked first.
  std::vector<std::string> cycle;
  FaultTree::DetectCycle(top_event_, &cycle);
  if (!cycle.empty()) {
    std::string msg = "Detected a cycle in '" + name_ + "' fault tree:\n";
    msg += FaultTree::PrintCycle(cycle);
    throw ValidationError(msg);
  }
}

void FaultTree::SetupForAnalysis() {
  /// @todo This function may be more flexible and gather information
  ///       about the changed tree structure. Databases of gates might
  ///       update optionally.

  // Assumes that the tree is fully developed.
  // Assumes that there is no change in gate structure of the tree. If there
  // is a change in gate structure, then the gate information is invalid.
  primary_events_.clear();
  basic_events_.clear();
  ccf_events_.clear();
  house_events_.clear();
  // Gather all primary events belonging to this tree.
  FaultTree::GatherPrimaryEvents();

  // Recording number of original basic events before putting new CCF events.
  num_basic_events_ = basic_events_.size();

  // Gather CCF generated basic events.
  FaultTree::GatherCcfBasicEvents();
}

bool FaultTree::DetectCycle(const GatePtr& gate,
                            std::vector<std::string>* cycle) {
  if (gate->mark() == "") {
    gate->mark("temporary");
    const std::map<std::string, EventPtr>* children = &gate->children();
    std::map<std::string, EventPtr>::const_iterator it;
    for (it = children->begin(); it != children->end(); ++it) {
      GatePtr child_gate = boost::dynamic_pointer_cast<Gate>(it->second);
      if (child_gate) {
        if (!inter_events_.count(child_gate->id())) {
          implicit_gates_.insert(std::make_pair(child_gate->id(), child_gate));
          inter_events_.insert(std::make_pair(child_gate->id(), child_gate));
        }
        if (FaultTree::DetectCycle(child_gate, cycle)) {
          cycle->push_back(gate->name());
          return true;
        }
      }
    }
    gate->mark("permanent");
  } else if (gate->mark() == "temporary") {
    cycle->push_back(gate->name());
    return true;
  }
  return false;  // This also covers permanently marked gates.
}

std::string FaultTree::PrintCycle(const std::vector<std::string>& cycle) {
  assert(!cycle.empty());
  std::vector<std::string>::const_iterator it = cycle.begin();
  std::string cycle_start = *it;
  std::string result = "->" + cycle_start;
  for (++it; *it != cycle_start; ++it) {
    result = "->" + *it + result;
  }
  result = cycle_start + result;
  return result;
}

void FaultTree::GatherPrimaryEvents() {
  FaultTree::GetPrimaryEvents(top_event_);

  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    FaultTree::GetPrimaryEvents(it->second);
  }
}

void FaultTree::GetPrimaryEvents(const GatePtr& gate) {
  const std::map<std::string, EventPtr>* children = &gate->children();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    if (!inter_events_.count(it->first)) {
      PrimaryEventPtr primary_event =
          boost::dynamic_pointer_cast<PrimaryEvent>(it->second);

      if (primary_event == 0) {  // The tree must be fully defined.
        throw LogicError("Node with id '" + it->second->name() +
                         "' was not defined in '" + name_+ "' tree");
      }

      primary_events_.insert(std::make_pair(it->first, primary_event));
      BasicEventPtr basic_event =
          boost::dynamic_pointer_cast<BasicEvent>(primary_event);
      if (basic_event) {
        basic_events_.insert(std::make_pair(it->first, basic_event));
        if (basic_event->HasCcf())
          ccf_events_.insert(std::make_pair(it->first, basic_event));
      } else {
        HouseEventPtr house_event =
            boost::dynamic_pointer_cast<HouseEvent>(primary_event);
        assert(house_event);
        house_events_.insert(std::make_pair(it->first, house_event));
      }
    }
  }
}

void FaultTree::GatherCcfBasicEvents() {
  boost::unordered_map<std::string, BasicEventPtr>::iterator it_b;
  for (it_b = ccf_events_.begin(); it_b != ccf_events_.end(); ++it_b) {
    assert(it_b->second->HasCcf());
    const std::map<std::string, EventPtr>* children =
        &it_b->second->ccf_gate()->children();
    std::map<std::string, EventPtr>::const_iterator it;
    for (it = children->begin(); it != children->end(); ++it) {
      BasicEventPtr basic_event =
          boost::dynamic_pointer_cast<BasicEvent>(it->second);
      assert(basic_event);
      basic_events_.insert(std::make_pair(basic_event->id(), basic_event));
      primary_events_.insert(std::make_pair(basic_event->id(), basic_event));
    }
  }
}

}  // namespace scram
