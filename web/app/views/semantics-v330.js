(function (global) {
  'use strict';
  var App = global.AIRApp;

  App.registerView('semantics', function () {
    var root = App.el('section');

    function version(v) {
      if (!v || typeof v !== 'object') return '—';
      return [v.major, v.minor, v.patch].join('.');
    }

    function render() {
      var snapshot = App.state.semantics || {};
      var implementations = Array.isArray(snapshot.implementations) ? snapshot.implementations : [];
      var packages = Array.isArray(snapshot.packages) ? snapshot.packages : [];
      var resolved = 0;
      var missing = 0;
      packages.forEach(function (pkg) {
        resolved += Number(pkg.resolved_count) || 0;
        missing += Number(pkg.missing_count) || 0;
      });

      App.clear(root);
      root.appendChild(App.pageHead(
        'Semantic capability authority',
        'Semantics & Extensions',
        'Trusted implementations and MissingSemantic state come directly from GET /semantics. Missing capability is structured state, not a generic runtime failure.'
      ));

      var cards = App.el('div', 'grid grid-4');
      cards.appendChild(App.metricCard('Trusted implementations', App.formatNumber(implementations.length, 0), 'already compiled/registered host code'));
      cards.appendChild(App.metricCard('Packages', App.formatNumber(packages.length, 0), 'data-only declarations'));
      cards.appendChild(App.metricCard('Resolved requirements', App.formatNumber(resolved, 0), 'exact contract matches'));
      cards.appendChild(App.metricCard('Missing requirements', App.formatNumber(missing, 0), 'no compatibility guessing'));
      root.appendChild(cards);

      var implCard = App.el('section', 'card padded');
      implCard.appendChild(App.el('div', 'eyebrow', 'Registered trusted implementations'));
      if (!implementations.length) {
        implCard.appendChild(App.el('div', 'empty', 'No semantic implementations reported.'));
      } else {
        implementations.forEach(function (item) {
          var row = App.el('div', 'semantic-row');
          var title = App.el('div');
          title.appendChild(App.el('strong', '', item.semantic_id || 'semantic'));
          title.appendChild(App.el('small', '', (item.kind || 'unknown') + ' contract ' + version(item.contract_version)));
          row.appendChild(title);
          var meta = App.el('div', 'semantic-meta');
          meta.appendChild(App.el('span', 'badge good', 'resolved'));
          meta.appendChild(App.el('code', '', item.implementation_id || 'implementation'));
          meta.appendChild(App.el('span', 'badge', item.origin || 'unknown origin'));
          row.appendChild(meta);
          implCard.appendChild(row);
        });
      }
      root.appendChild(implCard);

      packages.forEach(function (pkg) {
        var card = App.el('section', 'card padded');
        var title = App.el('div', 'section-title');
        title.appendChild(App.el('div', 'eyebrow', pkg.package_id || 'semantic package'));
        var counts = App.el('div', 'status-row');
        counts.appendChild(App.el('span', 'badge good', String(pkg.resolved_count || 0) + ' resolved'));
        counts.appendChild(App.el('span', (Number(pkg.missing_count) || 0) ? 'badge warn' : 'badge good', String(pkg.missing_count || 0) + ' missing'));
        title.appendChild(counts); card.appendChild(title);

        var requirements = Array.isArray(pkg.requirements) ? pkg.requirements : [];
        var wrap = App.el('div', 'table-wrap');
        var table = App.el('table', 'table');
        var hr = App.el('tr');
        ['Semantic', 'Kind', 'Contract', 'State', 'Implementation / missing reason'].forEach(function (name) { hr.appendChild(App.el('th', '', name)); });
        var thead = App.el('thead'); thead.appendChild(hr); table.appendChild(thead);
        var tbody = App.el('tbody');
        requirements.forEach(function (req) {
          var row = App.el('tr');
          row.appendChild(App.el('td', '', req.semantic_id || '—'));
          row.appendChild(App.el('td', '', req.kind || '—'));
          row.appendChild(App.el('td', '', version(req.contract_version)));
          var statusCell = App.el('td');
          statusCell.appendChild(App.el('span', req.resolved ? 'badge good' : 'badge warn', req.resolved ? 'resolved' : 'missing'));
          row.appendChild(statusCell);
          row.appendChild(App.el('td', '', req.resolved ? (req.implementation_id || 'registered') : (req.missing_reason || 'not registered')));
          tbody.appendChild(row);
        });
        table.appendChild(tbody); wrap.appendChild(table); card.appendChild(wrap); root.appendChild(card);
      });

      root.appendChild(App.callout('', 'Security boundary', 'Semantic packages are data-only. AIR does not execute package scripts, commands, entrypoints, Python modules, or library paths.'));

      if (App.state.uiMode === 'research') {
        var raw = App.el('details', 'card padded research-only');
        raw.appendChild(App.el('summary', '', 'Raw /semantics'));
        raw.appendChild(App.el('pre', 'code', App.json(snapshot)));
        root.appendChild(raw);
      }
    }

    document.addEventListener('air:state', render);
    render();
    return root;
  });
}(window));
