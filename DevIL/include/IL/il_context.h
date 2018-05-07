#pragma once

class ILcontext
{
public:
    class Impl;
    Impl* impl;

	ILcontext();
};