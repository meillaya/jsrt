function Box(v) {
    this.value = v;
}

Box.prototype = {
    get: function () {
        return this.value;
    }
};

let box = new Box(11);
box.get() + ":" + box["get"]();
