class Box {
    constructor(v) {
        this.value = v;
    }

    get() {
        return this.value;
    }
}

let box = new Box(7);
box.get() + ":" + (box.get == Box.prototype.get);
