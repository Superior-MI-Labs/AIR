(function (global) {
  'use strict';
  var App = global.AIRApp;
  App.registerView('models', function () {
    var root = App.el('section');
    function render() {
      var m = App.state.model || {};
      App.clear(root);
      root.appendChild(App.pageHead('Workload truth', 'Workload & Architecture', 'AIR owns the active executable model while shared execution contracts also describe the qualified FLUX.2 discriminator. The browser does not maintain a model registry.'));
      var grid = App.el('div', 'grid grid-2');
      var current = App.el('section', 'card padded');
      current.appendChild(App.el('div', 'eyebrow', 'Active model'));
      current.appendChild(App.el('h3', '', m.id || 'No model reported'));
      current.appendChild(App.kv('Architecture', m.architecture));
      current.appendChild(App.kv('Format', m.format));
      current.appendChild(App.kv('Backend', m.backend));
      current.appendChild(App.kv('Layers', m.layers));
      current.appendChild(App.kv('Embedding', m.embedding));
      current.appendChild(App.kv('Context length', m.context_length));
      current.appendChild(App.kv('Vocabulary size', m.vocabulary_size));
      grid.appendChild(current);
      var support = App.el('section', 'card padded stack');
      support.appendChild(App.el('div', 'eyebrow', 'AIR 0.11 candidate scope'));
      support.appendChild(App.el('h3', '', 'Qwen2 executable · FLUX.2 structural discriminator'));
      support.appendChild(App.el('p', '', 'Qwen2-family GGUF remains AIR-executable. FLUX.2 Klein lowers through the shared workload, resource, semantic, and ExecutionGraph R1 contracts; only deterministic latent-geometry and schedule semantics execute inside AIR today.'));
      support.appendChild(App.callout('warn', 'No browser hot-swap', 'AIR 0.11 does not yet expose a runtime model-load/hot-swap endpoint. Change models by restarting the canonical server through the launcher or future air-setup helper.'));
      grid.appendChild(support);
      root.appendChild(grid);
    }
    document.addEventListener('air:state', render); render(); return root;
  });
}(window));
