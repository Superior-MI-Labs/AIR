(function (global) {
  'use strict';

  var App = global.AIRApp;
  var NAV = [
    ['home', 'Overview'],
    ['playground', 'Playground'],
    ['decision', 'Decision'],
    ['models', 'Workload'],
    ['machine', 'Machine'],
    ['runtime', 'Runtime'],
    ['plan', 'Plan Lab'],
    ['execution', 'Execution'],
    ['semantics', 'Semantics'],
    ['diagnostics', 'Events'],
    ['metrics', 'Metrics'],
    ['setup', 'Setup'],
    ['about', 'About']
  ];
  var currentRoute = 'home';
  var viewNodes = {};
  var navButtons = {};
  var pollTimer = null;
  var polling = false;
  var structuralTick = 0;

  if (!App || !App.api) {
    throw new Error('AIR application dependencies did not load in the required order');
  }

  function mergeEvents(events) {
    var all = App.state.eventHistory.slice();
    var seen = {};
    var i;
    for (i = 0; i < all.length; i += 1) {
      if (all[i] && all[i].sequence != null) seen[String(all[i].sequence)] = true;
    }
    if (!Array.isArray(events)) return all;
    for (i = 0; i < events.length; i += 1) {
      if (!events[i]) continue;
      if (events[i].sequence == null || !seen[String(events[i].sequence)]) {
        all.push(events[i]);
        if (events[i].sequence != null) seen[String(events[i].sequence)] = true;
      }
    }
    if (all.length > 500) all = all.slice(all.length - 500);
    return all;
  }

  function routeFromHash() {
    var id = String(global.location.hash || '').replace(/^#\/?/, '');
    if (!App.views[id]) return 'home';
    return id;
  }

  function navigate(route) {
    var id = App.views[route] ? route : 'home';
    var key;
    currentRoute = id;
    for (key in viewNodes) {
      if (Object.prototype.hasOwnProperty.call(viewNodes, key)) {
        viewNodes[key].classList.toggle('hidden', key !== id);
      }
    }
    for (key in navButtons) {
      if (Object.prototype.hasOwnProperty.call(navButtons, key)) {
        navButtons[key].classList.toggle('active', key === id);
      }
    }
    if (global.location.hash !== '#' + id) global.location.hash = id;
    App.state.route = id;
    App.emit('air:route');
    if (id === 'execution') pollAir();
  }

  function modeButton() {
    var button = App.el('button', 'mode-toggle');
    button.type = 'button';
    button.id = 'ui-mode-toggle';
    button.addEventListener('click', function () {
      App.setUiMode(App.state.uiMode === 'research' ? 'novice' : 'research');
    });
    return button;
  }

  function buildShell() {
    var mount = document.getElementById('app');
    var shell = App.el('div', 'shell');
    var sidebar = App.el('aside', 'sidebar');
    var brand = App.el('div', 'brand');
    var brandMark = App.el('div', 'mini-mark', '≈+');
    var brandCopy = App.el('div');
    brandCopy.appendChild(App.el('strong', '', 'AIR'));
    brandCopy.appendChild(App.el('small', '', 'Control Room'));
    brand.appendChild(brandMark);
    brand.appendChild(brandCopy);
    sidebar.appendChild(brand);

    var nav = App.el('nav', 'nav');
    NAV.forEach(function (item) {
      var button = App.el('button');
      button.type = 'button';
      button.appendChild(App.el('span', '', item[1]));
      button.addEventListener('click', function () { navigate(item[0]); });
      navButtons[item[0]] = button;
      nav.appendChild(button);
    });
    sidebar.appendChild(nav);

    var foot = App.el('div', 'sidebar-foot');
    foot.appendChild(App.el('strong', '', 'AIR 0.11 · WEB 3.3'));
    foot.appendChild(document.createElement('br'));
    foot.appendChild(document.createTextNode('Adaptive execution · hosted-qualified candidate'));
    sidebar.appendChild(foot);

    var main = App.el('main', 'app-main');
    var top = App.el('header', 'topbar');
    var connection = App.el('div', 'connection');
    var dot = App.el('span', 'dot'); dot.id = 'connection-dot';
    var connectionText = App.el('span', '', 'Checking AIR'); connectionText.id = 'connection-text';
    connection.appendChild(dot);
    connection.appendChild(connectionText);
    top.appendChild(connection);
    top.appendChild(App.el('div', 'top-chip', 'Web 3.3.0'));
    var backend = App.el('div', 'top-chip', 'Backend: —'); backend.id = 'backend-chip'; top.appendChild(backend);
    var model = App.el('div', 'top-chip', 'Model: —'); model.id = 'model-chip'; top.appendChild(model);
    top.appendChild(App.el('div', 'top-spacer'));
    top.appendChild(modeButton());
    main.appendChild(top);

    var content = App.el('div', 'content');
    NAV.forEach(function (item) {
      var factory = App.views[item[0]];
      if (typeof factory !== 'function') throw new Error('Missing AIR view: ' + item[0]);
      var node = factory();
      node.dataset.view = item[0];
      node.classList.add('hidden');
      viewNodes[item[0]] = node;
      content.appendChild(node);
    });
    main.appendChild(content);
    shell.appendChild(sidebar);
    shell.appendChild(main);
    App.clear(mount);
    mount.appendChild(shell);

    document.addEventListener('air:state', updateTopbar);
    global.addEventListener('hashchange', function () { navigate(routeFromHash()); });
    navigate(routeFromHash());
  }

  function updateTopbar() {
    var s = App.state;
    var dot = document.getElementById('connection-dot');
    var text = document.getElementById('connection-text');
    var backend = document.getElementById('backend-chip');
    var model = document.getElementById('model-chip');
    var mode = document.getElementById('ui-mode-toggle');
    if (!dot || !text) return;
    dot.className = 'dot ' + (s.connected ? 'good' : 'bad');
    text.textContent = s.connected ? 'AIR connected' : 'AIR disconnected';
    if (backend) backend.textContent = 'Backend: ' + ((s.runtime && s.runtime.backend) || (s.health && s.health.backend) || (s.model && s.model.backend) || '—');
    if (model) model.textContent = 'Model: ' + ((s.model && s.model.id) || '—');
    if (mode) {
      mode.textContent = s.uiMode === 'research' ? 'Research mode' : 'Novice mode';
      mode.className = 'mode-toggle ' + (s.uiMode === 'research' ? 'research' : 'novice');
    }
    document.documentElement.dataset.airMode = s.uiMode;
  }

  function noteError(label, error) {
    App.state.lastError = label + ': ' + (error && error.message ? error.message : String(error));
  }

  function fetchStructural() {
    return Promise.all([
      App.api.machine().catch(function (error) { noteError('Machine endpoint', error); return null; }),
      App.api.environment().catch(function (error) { noteError('Environment endpoint', error); return null; }),
      App.api.semantics().catch(function (error) { noteError('Semantics endpoint', error); return null; })
    ]).then(function (results) {
      if (results[0]) App.state.machine = results[0];
      if (results[1]) App.state.environment = results[1];
      if (results[2]) App.state.semantics = results[2];
    });
  }

  function shouldFetchResearchEvidence() {
    return App.state.uiMode === 'research' || currentRoute === 'execution';
  }

  function fetchResearchEvidence() {
    if (!shouldFetchResearchEvidence()) return Promise.resolve();
    return Promise.all([
      App.api.timeline().catch(function (error) { noteError('Timeline endpoint', error); return null; }),
      App.api.executionGraphs().catch(function (error) { noteError('Execution graphs endpoint', error); return null; })
    ]).then(function (results) {
      if (results[0]) App.state.timeline = results[0];
      if (results[1]) App.state.executionGraphs = results[1];
    });
  }

  function pollAir() {
    if (polling) return;
    polling = true;
    App.api.health().then(function (health) {
      App.state.connected = health && health.status === 'ok';
      App.state.health = health;
      App.state.lastError = null;
      return Promise.all([
        App.api.model().catch(function (error) { noteError('Model endpoint', error); return null; }),
        App.api.runtime().catch(function (error) { noteError('Runtime endpoint', error); return null; }),
        App.api.events().catch(function (error) { noteError('Events endpoint', error); return null; }),
        App.api.metrics().catch(function (error) { noteError('Metrics endpoint', error); return ''; })
      ]);
    }).then(function (results) {
      if (results) {
        if (results[0]) App.state.model = results[0];
        if (results[1]) App.state.runtime = results[1];
        if (results[2]) {
          App.state.events = Array.isArray(results[2]) ? results[2] : [];
          App.state.eventHistory = mergeEvents(App.state.events);
        }
        if (typeof results[3] === 'string') App.state.metricsText = results[3];
      }

      structuralTick += 1;
      var structural = (structuralTick === 1 || structuralTick % 5 === 0)
        ? fetchStructural()
        : Promise.resolve();
      return structural.then(fetchResearchEvidence);
    }).then(function () {
      App.emit('air:state');
    }).catch(function (error) {
      App.state.connected = false;
      App.state.health = null;
      App.state.lastError = error && error.message ? error.message : String(error);
      App.emit('air:state');
    }).finally(function () {
      polling = false;
      schedulePoll();
    });
  }

  function schedulePoll() {
    if (pollTimer) clearTimeout(pollTimer);
    pollTimer = setTimeout(pollAir, document.hidden ? 8000 : 2000);
  }

  document.addEventListener('visibilitychange', schedulePoll);
  document.addEventListener('air:state', function () {
    if (App.state.uiMode === 'research' && !polling) schedulePoll();
  });
  document.addEventListener('DOMContentLoaded', function () {
    buildShell();
    global.__AIR_WEB_BOOT.started = true;
    pollAir();
  });
}(window));
