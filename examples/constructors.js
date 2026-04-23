function Person(name) {
  this.name = name;
}

Person.prototype = {
  greet: function () {
    return "hello " + this.name;
  }
};

let ada = new Person("Ada");
print(ada.greet());
