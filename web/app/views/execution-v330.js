(function (global) {
  'use strict';
  var App = global.AIRApp;

  App.registerView('execution', function () {
    var root = App.el('section');

    function durationMs(span) {
      var start = Number(span && span.start_ns);
      var end = Number(span && span.end_ns);
      if (!isFinite(start) || !isFinite(end) || end < start) return '—';
      return App.formatNumber((end - start) / 1000000, 3) + ' ms';
    }

    function badgeForEvidence(status) {
      if (status === 'concordant') return 'badge good';
      if (status === 'contradictory') return 'badge bad';
      if (status === 'incomplete') return 'badge warn';
      return 'badge';
    }

    function render() {
      var timeline = App.state.timeline || {};
      var graphs = App.state.executionGraphs || {};
      var spans = Array.isArray(timeline.spans) ? timeline.spans : [];
      var observations = Array.isArray(graphs.observations) ? graphs.observations : [];

      App.clear(root);
      root.appendChild(App.pageHead(
        'Execution evidence',
        'Execution',
        'Timeline spans and ExecutionGraph observations are evidence. Descriptive graphs are not presented as executed work.'
      ));

      if (!spans.length && App.state.uiMode !== 'research') {
        root.appendChild(App.callout('', 'Novice mode', 'Detailed timeline/graph evidence is loaded when this view is open and continuously in Research mode.'));
      }

      var cards = App.el('div', 'grid grid-4');
      cards.appendChild(App.metricCard('Observation level', timeline.level || 'Not reported', 'canonical /timeline'));
      cards.appendChild(App.metricCard('Spans', App.formatNumber(spans.length, 0), 'dropped ' + App.formatNumber(timeline.dropped_spans, 0)));
      cards.appendChild(App.metricCard('Graphs', App.formatNumber(observations.length, 0), 'derivation failures ' + App.formatNumber(graphs.derivation_failures, 0)));
      cards.appendChild(App.metricCard('Graph topology', graphs.topology_status || 'Not reported', graphs.topology_fingerprint || 'no fingerprint'));
      root.appendChild(cards);

      var graphCard = App.el('section', 'card padded');
      graphCard.appendChild(App.el('div', 'eyebrow', 'ExecutionGraph evidence'));
      var graphList = App.el('div', 'stack');
      if (!observations.length) graphList.appendChild(App.el('div', 'empty', 'No ExecutionGraph observations received.'));
      observations.slice(-24).reverse().forEach(function (observation) {
        var graph = observation.graph || {};
        var item = App.el('div', 'evidence-row');
        var head = App.el('div', 'evidence-head');
        head.appendChild(App.el('strong', '', graph.workload_kind || graph.invocation || 'graph'));
        head.appendChild(App.el('span', badgeForEvidence(observation.evidence_status), observation.evidence_status || 'not-evaluated'));
        item.appendChild(head);
        var meta = App.el('div', 'evidence-meta');
        meta.appendChild(App.el('span', '', 'binding ' + (graph.binding || '—')));
        meta.appendChild(App.el('span', '', 'backend ' + (graph.backend || '—')));
        meta.appendChild(App.el('span', '', 'unit ' + (graph.work_unit_kind || '—')));
        meta.appendChild(App.el('span', '', graph.identity || 'no identity'));
        item.appendChild(meta);
        if (graph.binding === 'descriptive') {
          item.appendChild(App.callout('warn', 'Descriptive graph', 'This graph describes qualified structure. AIR has not claimed this region executed.'));
        }
        graphList.appendChild(item);
      });
      graphCard.appendChild(graphList); root.appendChild(graphCard);

      var timelineCard = App.el('section', 'card padded');
      timelineCard.appendChild(App.el('div', 'eyebrow', 'Recent execution timeline'));
      var wrap = App.el('div', 'table-wrap');
      var table = App.el('table', 'table');
      var headRow = App.el('tr');
      ['Seq', 'Scope', 'Category', 'Phase', 'Backend', 'Work', 'Duration', 'OK'].forEach(function (name) {
        headRow.appendChild(App.el('th', '', name));
      });
      var thead = App.el('thead'); thead.appendChild(headRow); table.appendChild(thead);
      var tbody = App.el('tbody');
      if (!spans.length) {
        var er = App.el('tr'); var ec = App.el('td', '', 'No spans received.'); ec.colSpan = 8; er.appendChild(ec); tbody.appendChild(er);
      }
      spans.slice(-100).reverse().forEach(function (span) {
        var row = App.el('tr');
        var work = Number(span.work_units) > 0 ? String(span.work_units) + ' ' + (span.work_unit_kind || '?') : '—';
        [span.observation_sequence, span.scope, span.category, span.phase, span.backend, work, durationMs(span), span.success === false ? 'no' : 'yes'].forEach(function (value) {
          row.appendChild(App.el('td', '', value == null ? '—' : value));
        });
        tbody.appendChild(row);
      });
      table.appendChild(tbody); wrap.appendChild(table); timelineCard.appendChild(wrap); root.appendChild(timelineCard);

      if (App.state.uiMode === 'research') {
        var raw = App.el('div', 'grid grid-2 research-only');
        var t = App.el('details', 'card padded'); t.appendChild(App.el('summary', '', 'Raw /timeline')); t.appendChild(App.el('pre', 'code', App.json(timeline))); raw.appendChild(t);
        var g = App.el('details', 'card padded'); g.appendChild(App.el('summary', '', 'Raw /execution-graphs')); g.appendChild(App.el('pre', 'code', App.json(graphs))); raw.appendChild(g);
        root.appendChild(raw);
      }
    }

    document.addEventListener('air:state', render);
    render();
    return root;
  });
}(window));
