(function (global) {
  'use strict';
  var App = global.AIRApp;

  App.registerView('machine', function () {
    var root = App.el('section');

    function resourceMap(environment) {
      var map = {};
      var resources = environment && Array.isArray(environment.resources) ? environment.resources : [];
      resources.forEach(function (item) {
        if (item && item.node_id != null) map[String(item.node_id)] = item;
      });
      return map;
    }

    function render() {
      var topology = App.state.machine || {};
      var environment = App.state.environment || {};
      var nodes = Array.isArray(topology.nodes) ? topology.nodes : [];
      var links = Array.isArray(topology.links) ? topology.links : [];
      var available = resourceMap(environment);
      var observed = environment.observed_unix_ms ? new Date(Number(environment.observed_unix_ms)).toLocaleTimeString() : 'Not reported';

      App.clear(root);
      root.appendChild(App.pageHead(
        'Machine authority',
        'Machine',
        'Stable topology comes from GET /machine. Dynamic capacity comes from GET /environment. The Control Room does not merge those authorities.'
      ));

      var cards = App.el('div', 'grid grid-4');
      cards.appendChild(App.metricCard('Topology nodes', App.formatNumber(nodes.length, 0), 'stable structural inventory'));
      cards.appendChild(App.metricCard('Links', App.formatNumber(links.length, 0), 'measured flag preserved'));
      cards.appendChild(App.metricCard('Topology schema', topology.schema_version == null ? '—' : topology.schema_version, 'fingerprint-scoped'));
      cards.appendChild(App.metricCard('Environment observed', observed, 'dynamic snapshot'));
      root.appendChild(cards);

      var fingerprint = App.el('section', 'card padded stack');
      fingerprint.appendChild(App.el('div', 'eyebrow', 'Topology identity'));
      fingerprint.appendChild(App.el('code', 'code', topology.fingerprint || 'Topology has not been reported yet.'));
      if (environment.topology_fingerprint && topology.fingerprint &&
          environment.topology_fingerprint !== topology.fingerprint) {
        fingerprint.appendChild(App.callout('bad', 'Topology mismatch', 'The dynamic environment snapshot references a different topology fingerprint.'));
      }
      root.appendChild(fingerprint);

      var tableCard = App.el('section', 'card padded');
      tableCard.appendChild(App.el('div', 'eyebrow', 'Resources'));
      var wrap = App.el('div', 'table-wrap');
      var table = App.el('table', 'table');
      var thead = App.el('thead');
      var hr = App.el('tr');
      ['Resource', 'Kind', 'Backend', 'Architecture', 'Total', 'Available', 'Capabilities'].forEach(function (name) {
        hr.appendChild(App.el('th', '', name));
      });
      thead.appendChild(hr); table.appendChild(thead);
      var tbody = App.el('tbody');
      if (!nodes.length) {
        var emptyRow = App.el('tr');
        var emptyCell = App.el('td', '', 'No canonical machine topology received.');
        emptyCell.colSpan = 7;
        emptyRow.appendChild(emptyCell); tbody.appendChild(emptyRow);
      }
      nodes.forEach(function (node) {
        var row = App.el('tr');
        var env = available[String(node.id)] || {};
        var title = (node.name || node.id || 'resource') + (node.ordinal == null ? '' : ' #' + node.ordinal);
        [
          title,
          node.kind || '—',
          node.backend || '—',
          node.architecture || '—',
          App.formatBytes(node.total_bytes),
          env.available_bytes == null ? 'Not observed' : App.formatBytes(env.available_bytes),
          Array.isArray(node.capabilities) && node.capabilities.length ? node.capabilities.join(', ') : '—'
        ].forEach(function (value) { row.appendChild(App.el('td', '', value)); });
        tbody.appendChild(row);
      });
      table.appendChild(tbody); wrap.appendChild(table); tableCard.appendChild(wrap); root.appendChild(tableCard);

      if (App.state.uiMode === 'research') {
        var raw = App.el('div', 'grid grid-2 research-only');
        var machineRaw = App.el('details', 'card padded');
        machineRaw.appendChild(App.el('summary', '', 'Raw /machine'));
        machineRaw.appendChild(App.el('pre', 'code', App.json(topology)));
        raw.appendChild(machineRaw);
        var environmentRaw = App.el('details', 'card padded');
        environmentRaw.appendChild(App.el('summary', '', 'Raw /environment'));
        environmentRaw.appendChild(App.el('pre', 'code', App.json(environment)));
        raw.appendChild(environmentRaw);
        root.appendChild(raw);
      }
    }

    document.addEventListener('air:state', render);
    render();
    return root;
  });
}(window));
