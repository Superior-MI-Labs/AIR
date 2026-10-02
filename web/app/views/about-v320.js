(function (global) {
  'use strict';

  var App = global.AIRApp;

  App.registerView('about', function () {
    var root = App.el('section');

    root.appendChild(App.pageHead(
      'Superior MI Labs · R&D',
      'About AIR',
      'AIR is an experimental inference runtime built around explicit ownership, falsifiable optimization, and one canonical production execution path.'
    ));

    var hero = App.el('section', 'card hero r-and-d-hero');
    hero.appendChild(App.el('div', 'eyebrow', 'AIR 0.11 candidate · PUBLIC R&D RELEASE'));
    hero.appendChild(App.el('h2', '', 'Inference as systems engineering'));
    hero.appendChild(App.el('p', '', 'Instead of treating inference as a black box, AIR makes scheduling, state, admission, backend execution, cancellation, and observability explicit enough to test and reason about.'));
    var badges = App.el('div', 'status-row');
    badges.appendChild(App.el('span', 'badge blue', 'C++20'));
    badges.appendChild(App.el('span', 'badge blue', 'GGUF'));
    badges.appendChild(App.el('span', 'badge', 'Reference + CUDA'));
    badges.appendChild(App.el('span', 'badge warn', 'Research software'));
    hero.appendChild(badges);
    root.appendChild(hero);

    var principles = App.el('div', 'grid grid-3');
    [
      ['One production path', 'The browser, HTTP server, benchmarks, and qualification all converge on the same inference service rather than parallel implementations.'],
      ['State has an owner', 'Sequence state, resource admission, and runtime truth stay authoritative in AIR instead of being duplicated across UI or helper layers.'],
      ['Evidence beats intuition', 'Optimizations are retained only after correctness and machine evidence survive qualification. Negative results remain part of the design history.']
    ].forEach(function (item) {
      var card = App.el('section', 'card padded stack');
      card.appendChild(App.el('div', 'eyebrow', item[0]));
      card.appendChild(App.el('p', '', item[1]));
      principles.appendChild(card);
    });
    root.appendChild(principles);

    var architecture = App.el('section', 'card padded stack');
    architecture.appendChild(App.el('div', 'eyebrow', 'Architecture'));
    architecture.appendChild(App.el('h3', '', 'One authoritative route from model to surface'));
    var flow = App.el('div', 'architecture-flow');
    ['Package / Model semantics', 'Workload adapter', 'ExecutionGraph R1', 'Planner / Resource authority', 'InferenceService', 'Reference / CUDA implementations', 'Evidence', 'HTTP / Control Room'].forEach(function (name, i, arr) {
      flow.appendChild(App.el('div', 'architecture-node', name));
      if (i < arr.length - 1) flow.appendChild(App.el('div', 'architecture-arrow', '↓'));
    });
    architecture.appendChild(flow);
    root.appendChild(architecture);

    var status = App.el('div', 'grid grid-2');
    var now = App.el('section', 'card padded stack');
    now.appendChild(App.el('div', 'eyebrow', 'Where AIR is now'));
    now.appendChild(App.el('h3', '', '0.11 hosted-qualified release candidate'));
    now.appendChild(App.el('p', '', 'The candidate preserves the earlier qualified Qwen runtime and adds machine/environment separation, typed execution evidence, adaptive strategy planning, ExecutionGraph R1, a FLUX.2 second-architecture discriminator, trusted semantic extensions, and this Control Room.'));
    now.appendChild(App.callout('', 'Important', 'Hosted qualification is not a substitute for the pending post-R1 NVIDIA replay, AIR-owned FLUX model/device execution, image parity, or human usability testing.'));
    status.appendChild(now);

    var limits = App.el('section', 'card padded stack');
    limits.appendChild(App.el('div', 'eyebrow', 'Explicit V1 limits'));
    var list = App.el('ul', 'clean-list');
    [
      'Decision scores are relative candidate-set-normalized values, not calibrated confidence.',
      'qualified-auto scoring and abstention thresholds are not qualified.',
      'Persistent CUDA cross-request state retention remains deferred.',
      'Model support is intentionally narrower than arbitrary GGUF compatibility.'
    ].forEach(function (line) { list.appendChild(App.el('li', '', line)); });
    limits.appendChild(list);
    status.appendChild(limits);
    root.appendChild(status);

    var lab = App.el('section', 'card padded stack');
    lab.appendChild(App.el('div', 'eyebrow', 'Superior MI Labs'));
    lab.appendChild(App.el('h3', '', 'Early-stage AI systems R&D from Upper Michigan'));
    lab.appendChild(App.el('p', '', 'Superior MI Labs is exploring inference architecture, reasoning systems, local compute infrastructure, and software that can eventually support developers, education, entrepreneurs, businesses, and communities.'));
    lab.appendChild(App.el('p', '', 'The project is early. Public releases are intended to make the work testable, inspectable, and easier to collaborate on.'));
    root.appendChild(lab);

    return root;
  });
}(window));
