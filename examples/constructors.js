let proto = {
  get: function () {
    return this.value;
  }
};

function Box(value) {
  this.value = value;
}

Box.prototype = proto;

function Replace() {
  return {value: 9};
}

let box = new Box(7);
let replacement = new Replace();

print(box.get() + ":" + replacement.value + ":" + (this == null));
