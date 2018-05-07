#include "il_context_impl.h"

ILcontext::ILcontext() :
    impl(new Impl())
{
    this->impl->iCurImage = NULL;
}