'use strict';
'require form';
'require network';
'require tools.widgets as widgets';
'require view';

return view.extend({
	load: function() {
		return Promise.all([
			network.getNetworks(),
		]);
	},

    render: function (loaded_promises) {
		    let m, s, o;
		    const networks = loaded_promises[0];

		    m = new form.Map('zmq-proxy', _('ZMQ Proxy Service'),
			                   _('ZMQ Proxy Service: allows you to forward and route ZMQ PUB/SUB networks (cf: <a target="_blank" href="https://zguide.zeromq.org/docs/chapter2/#The-Dynamic-Discovery-Problem">Pub-Sub Network with Proxy</a>). Multiple proxy services can be setup for finer grained control.'));

		    s = m.section(form.TypedSection, 'zmq-proxy', _('ZMQ Proxy Configuration'));
		    s.anonymous = true;
		    s.addremove = true;

        o = s.option(form.ListValue, 'pub_method', _('Publisher Connection Method'));
		    o.value('bind', _('Bind'));
		    o.value('connect', _('Connect'));
		    o.default = 'bind';

		    o = s.option(form.DynamicList,
			               'pubs',
			               _('Publisher Endpoint(s)'),
			               _('ZMQ Publisher Endpoint(s) to proxy.'));
		    o.datatype = 'string';

        o = s.option(form.ListValue, 'sub_method', _('Subscriber Connection Method'));
		    o.value('bind', _('Bind'));
		    o.value('connect', _('Connect'));
		    o.default = 'bind';

		    o = s.option(form.DynamicList,
			               'subs',
			               _('Subscriber Endpoint(s)'),
			               _('ZMQ Subscriber Endpoint(s) to proxy.'));
		    o.datatype = 'string';

		    o = s.option(form.DynamicList,
			               'subscriptions',
			               _('Subscriptions (filters)'));
		    o.datatype = 'string';
		    o.placeholder = 'tag1';

		    return m.render();
	  }

});
