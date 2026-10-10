#pragma once

class CreateFpmPagePort {
public:
  virtual ~CreateFpmPagePort() = default;
  virtual void initialize() = 0;
};
