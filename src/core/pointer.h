/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef NS_POINTER_H
#define NS_POINTER_H

#include "attribute.h"
#include "object.h"

namespace ns3 {

class PointerValue : public AttributeValue
{
public:
  PointerValue ();

  PointerValue (Ptr<Object> object);

  void SetObject (Ptr<Object> object);

  Ptr<Object> GetObject (void) const;

  template <typename T>
  PointerValue (const Ptr<T> &object);

  template <typename T>
  void Set (const Ptr<T> &object);

  template <typename T>
  Ptr<T> Get (void) const;

  template <typename T>
  operator Ptr<T> () const;

  virtual Ptr<AttributeValue> Copy (void) const;
  virtual std::string SerializeToString (Ptr<const AttributeChecker> checker) const;
  virtual bool DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker);

private:
  Ptr<Object> m_value;
};

template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (Ptr<U> T::*memberVariable);
template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (void (T::*setter) (Ptr<U>));
template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (Ptr<U> (T::*getter) (void) const);
template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (void (T::*setter) (Ptr<U>),
		     Ptr<U> (T::*getter) (void) const);
template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (Ptr<U> (T::*getter) (void) const,
		     void (T::*setter) (Ptr<U>));

class PointerChecker : public AttributeChecker 
{
public:
  virtual TypeId GetPointeeTypeId (void) const = 0;
};
template <typename T>
Ptr<AttributeChecker> MakePointerChecker (void);

} // namespace ns3

namespace ns3 {


namespace internal {

template <typename T>
class APointerChecker : public PointerChecker
{
  virtual bool Check (const AttributeValue &val) const {
    const PointerValue *value = dynamic_cast<const PointerValue *> (&val);
    if (value == 0)
      {
	return false;
      }
    if (value->GetObject () == 0)
      {
	return true;
      }
    T *ptr = dynamic_cast<T*> (PeekPointer (value->GetObject ()));
    if (ptr == 0)
      {
	return false;
      }
    return true;
  }
  virtual std::string GetType (void) const {
    // XXX: we should be able to return better information
    TypeId tid = T::GetTypeId ();
    return "Ptr< " + tid.GetName () + " >";
  }
  virtual bool HasTypeConstraints (void) const {
    return false;
  }
  virtual std::string GetTypeConstraints (void) const {
    return "";
  }
  virtual Ptr<AttributeValue> Create (void) const {
    return ns3::Create<PointerValue> ();
  }
  virtual bool Copy (const AttributeValue &source, AttributeValue &destination) const {
    const PointerValue *src = dynamic_cast<const PointerValue *> (&source);
    PointerValue *dst = dynamic_cast<PointerValue *> (&destination);
    if (src == 0 || dst == 0)
      {
        return false;
      }
    *dst = *src;
    return true;
  }
  virtual TypeId GetPointeeTypeId (void) const {
    return T::GetTypeId ();
  }
};

/********************************************************
 *              The Accessor associated to 
 *               PointerValue
 ********************************************************/

template <typename T, typename U>
class PointerAccessor : public AttributeAccessor
{
public:
  virtual ~PointerAccessor () {}
  virtual bool Set (ObjectBase * object, const AttributeValue &val) const {
      T *obj = dynamic_cast<T *> (object);
      if (obj == 0)
        {
          return false;
        }
      const PointerValue *value = dynamic_cast<const PointerValue *> (&val);
      if (value == 0)
        {
          return false;
        }
      Ptr<U> ptr = dynamic_cast<U*> (PeekPointer (value->GetObject ()));
      if (ptr == 0)
        {
          return false;
        }
      DoSet (obj, ptr);
      return true;
    }
  virtual bool Get (const ObjectBase * object, AttributeValue &val) const {
      const T *obj = dynamic_cast<const T *> (object);
      if (obj == 0)
        {
          return false;
        }
      PointerValue *value = dynamic_cast<PointerValue *> (&val);
      if (value == 0)
        {
          return false;
        }
      value->Set (DoGet (obj));
      return true;
    }
private:
  virtual void DoSet (T *object, Ptr<U> value) const = 0;
  virtual Ptr<U> DoGet (const T *object) const = 0;
};

} // namespace internal


template <typename T>
PointerValue::PointerValue (const Ptr<T> &object)
{
  m_value = object;
}

template <typename T>
void 
PointerValue::Set (const Ptr<T> &object)
{
  m_value = object;
}

template <typename T>
Ptr<T> 
PointerValue::Get (void) const
{
  T *v = dynamic_cast<T *> (PeekPointer (m_value));
  return v;
}

template <typename T>
PointerValue::operator Ptr<T> () const
{
  return Get<T> ();
}


template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (Ptr<U> T::*memberVariable)
{
  struct MemberVariable : public internal::PointerAccessor<T,U>
  {
    Ptr<U> T::*m_memberVariable;
    virtual void DoSet (T *object, Ptr<U> value) const {
      (object->*m_memberVariable) = value;
    }
    virtual Ptr<U> DoGet (const T *object) const {
      return object->*m_memberVariable;
    }
    virtual bool HasGetter (void) const {
      return true;
    }
    virtual bool HasSetter (void) const {
      return true;
    }
  } *spec = new MemberVariable ();
  spec->m_memberVariable = memberVariable;
  return Ptr<const AttributeAccessor> (spec, false);
}

template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (void (T::*setter) (Ptr<U>))
{
  struct MemberMethod : public internal::PointerAccessor<T,U>
  {
    void (T::*m_setter) (Ptr<U>);
    virtual void DoSet (T *object, Ptr<U> value) const {
      (object->*m_setter) (value);
    }
    virtual Ptr<U> DoGet (const T *object) const {
      return 0;
      //return (object->*m_getter) ();
    }
    virtual bool HasGetter (void) const {
      return false;
    }
    virtual bool HasSetter (void) const {
      return true;
    }
  } *spec = new MemberMethod ();
  spec->m_setter = setter;
  return Ptr<const AttributeAccessor> (spec, false);
}

template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (Ptr<U> (T::*getter) (void) const)
{
  struct MemberMethod : public internal::PointerAccessor<T,U>
  {
    Ptr<U> (T::*m_getter) (void) const;
    virtual void DoSet (T *object, Ptr<U> value) const {
      //(object->*m_setter) (value);
    }
    virtual Ptr<U> DoGet (const T *object) const {
      return (object->*m_getter) ();
    }
    virtual bool HasGetter (void) const {
      return true;
    }
    virtual bool HasSetter (void) const {
      return false;
    }
  } *spec = new MemberMethod ();
  spec->m_getter = getter;
  return Ptr<const AttributeAccessor> (spec, false);
}
template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (void (T::*setter) (Ptr<U>),
		     Ptr<U> (T::*getter) (void) const)
{
  return MakePointerAccessor (getter, setter);
}
template <typename T, typename U>
Ptr<const AttributeAccessor>
MakePointerAccessor (Ptr<U> (T::*getter) (void) const,
		     void (T::*setter) (Ptr<U>))
{
  struct MemberMethod : public internal::PointerAccessor<T,U>
  {
    void (T::*m_setter) (Ptr<U>);
    Ptr<U> (T::*m_getter) (void) const;
    virtual void DoSet (T *object, Ptr<U> value) const {
      (object->*m_setter) (value);
    }
    virtual Ptr<U> DoGet (const T *object) const {
      return (object->*m_getter) ();
    }
    virtual bool HasGetter (void) const {
      return true;
    }
    virtual bool HasSetter (void) const {
      return true;
    }
  } *spec = new MemberMethod ();
  spec->m_setter = setter;
  spec->m_getter = getter;
  return Ptr<const AttributeAccessor> (spec, false);
}

template <typename T>
Ptr<AttributeChecker>
MakePointerChecker (void)
{
  return Create<internal::APointerChecker<T> > ();
}


} // namespace ns3

#endif /* NS_POINTER_H */
