class HiggzSockets {
  constructor() {
    this.startSockets();
    this.fakeSocket();
  }

  startSockets() {
    this.sock = [];
  }

  fakeSocket() {
    this.sock.push('fake');
  }

  funkyCall() {
    this.myAdd();
  }
}

class Higgz extends HiggzSockets {
  constructor() {
    super();
    this.opts = {};

    this._eth_seq = 0;
    this.environment = undefined;
  }


  start(optsin) {
    this.bar = optsin.bar;

    this.fakeSocket();
  }


  callout() {
    console.log(this.bar);
    console.log(this.sock);
  }

  myAdd() {
    this.sock.push('myadd from parent');
  }
}




function main() {
  let h = new Higgz();

  h.start({bar:'hello world'});

  h.callout();

  h.funkyCall();

  console.log('--');
  h.callout();
}


main();