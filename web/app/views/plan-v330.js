(function (global) {
  'use strict';
  var App = global.AIRApp;

  App.registerView('plan', function () {
    var root = App.el('section');

    function render() {
      var runtime = App.state.runtime || {};
      var planner = runtime.planner || {};
      var candidates = Array.isArray(planner.candidates) ? planner.candidates : [];
      var resources = Array.isArray(runtime.current_prepared_resources) ? runtime.current_prepared_resources : [];

      App.clear(root);
      root.appendChild(App.pageHead(
        'Planner evidence',
        'Plan Lab',
        'This view renders Strategy Lab decisions already made by AIR. The browser does not score, rank, or select candidates.'
      ));

      var selected = App.el('section', 'card padded stack');
      selected.appendChild(App.el('div', 'eyebrow', 'Active plan'));
      selected.appendChild(App.el('h3', '', planner.strategy_id || 'No strategy reported'));
      selected.appendChild(App.kv('Planner mode', planner.mode));
      selected.appendChild(App.kv('Objective', planner.objective));
      selected.appendChild(App.kv('Decision reason', planner.decision_reason));
      selected.appendChild(App.kv('Selected backend', planner.selected_backend));
      selected.appendChild(App.kv('Prepared state hot', planner.prepared_state_hot == null ? null : String(planner.prepared_state_hot)));
      selected.appendChild(App.kv('Estimated transition', planner.estimated_transition_ms == null ? null : App.formatMs(planner.estimated_transition_ms)));
      selected.appendChild(App.kv('Break-even tokens', planner.estimated_break_even_tokens == null ? null : App.formatNumber(planner.estimated_break_even_tokens, 2)));
      root.appendChild(selected);

      var tableCard = App.el('section', 'card padded');
      tableCard.appendChild(App.el('div', 'eyebrow', 'Candidate trace'));
      var wrap = App.el('div', 'table-wrap');
      var table = App.el('table', 'table');
      var hr = App.el('tr');
      ['Strategy', 'Disposition', 'Eligible', 'Memory feasible', 'Hot', 'Transition', 'Horizon', 'Break-even', 'Prepared bytes'].forEach(function (name) {
        hr.appendChild(App.el('th', '', name));
      });
      var thead = App.el('thead'); thead.appendChild(hr); table.appendChild(thead);
      var tbody = App.el('tbody');
      if (!candidates.length) {
        var er = App.el('tr'); var ec = App.el('td', '', 'No Strategy Lab candidate trace reported.'); ec.colSpan = 9; er.appendChild(ec); tbody.appendChild(er);
      }
      candidates.forEach(function (candidate) {
        var row = App.el('tr');
        [
          candidate.strategy_id || '—',
          candidate.disposition || '—',
          candidate.eligible == null ? '—' : String(candidate.eligible),
          candidate.memory_feasible == null ? '—' : String(candidate.memory_feasible),
          candidate.prepared_state_hot == null ? '—' : String(candidate.prepared_state_hot),
          candidate.estimated_transition_ms == null ? '—' : App.formatMs(candidate.estimated_transition_ms),
          candidate.estimated_horizon_ms == null ? '—' : App.formatMs(candidate.estimated_horizon_ms),
          candidate.estimated_break_even_tokens == null ? '—' : App.formatNumber(candidate.estimated_break_even_tokens, 2),
          candidate.prepared_artifact_bytes == null ? '—' : App.formatBytes(candidate.prepared_artifact_bytes)
        ].forEach(function (value) { row.appendChild(App.el('td', '', value)); });
        tbody.appendChild(row);
      });
      table.appendChild(tbody); wrap.appendChild(table); tableCard.appendChild(wrap); root.appendChild(tableCard);

      var resourceCard = App.el('section', 'card padded');
      resourceCard.appendChild(App.el('div', 'eyebrow', 'Prepared resource residency'));
      if (!resources.length) {
        resourceCard.appendChild(App.el('div', 'empty', 'No identified prepared resources are currently reported.'));
      } else {
        resources.forEach(function (resource) {
          var row = App.el('div', 'kv');
          row.appendChild(App.el('span', '', resource.resource_id || 'resource'));
          row.appendChild(App.el('strong', '', (resource.state || 'unknown') + ' · ' + App.formatBytes(resource.device_bytes)));
          resourceCard.appendChild(row);
        });
      }
      root.appendChild(resourceCard);

      if (App.state.uiMode === 'research') {
        var raw = App.el('details', 'card padded research-only');
        raw.appendChild(App.el('summary', '', 'Raw planner/runtime JSON'));
        raw.appendChild(App.el('pre', 'code', App.json(runtime)));
        root.appendChild(raw);
      }
    }

    document.addEventListener('air:state', render);
    render();
    return root;
  });
}(window));
