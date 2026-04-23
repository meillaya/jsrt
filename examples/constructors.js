function Counter(start) {
    this.value = start;
}

Counter.prototype.bump = function () {
    this.value = this.value + 1;
    return this.value;
};

let counter = new Counter(4);
counter.bump() + ":" + counter.bump();
