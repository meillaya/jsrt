function Box(name) {
  this.name = name;
}

Box.prototype = {
  label: function () {
    return "box:" + this.name;
  }
};

let item = new Box("jsrt");
item.label();
