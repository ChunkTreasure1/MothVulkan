#pragma once

template<typename T>
class Ref
{
public:
	Ref();
	Ref(T* aPtr);
	Ref(Ref<T>& aRef);
	~Ref();

	Ref<T>& operator=(Ref<T>& aRef);
	Ref<T>& operator=(T* aPtr);

	bool operator==(const Ref<T>& aRef) const;
	bool operator!() const;

	void Release();

	explicit operator bool() const;

	T* operator->();
	T& operator*();

	T* Get();
	const size_t GetRefCount() const;

	template<typename V>
	Ref<V> As();

private:
	T* m_data = nullptr;
	size_t m_referenceCounter = 0;
	size_t* m_referenceCounterPtr = nullptr;
};

template<typename T>
inline Ref<T>::Ref()
{
	m_referenceCounterPtr = &m_referenceCounter;
	m_referenceCounter++;
}

template<typename T>
inline Ref<T>::Ref(T* aPtr)
{
	m_data = aPtr;
	m_referenceCounterPtr = &m_referenceCounter;
	m_referenceCounter++;
}

template<typename T>
inline Ref<T>::Ref(Ref<T>& aRef)
{
	*this = aRef;
}

template<typename T>
inline Ref<T>::~Ref()
{
	(*m_referenceCounterPtr)--;
	if ((*m_referenceCounterPtr) == 0)
	{
		delete m_data;
	}
}

template<typename T>
inline Ref<T>& Ref<T>::operator=(Ref<T>& aRef)
{
	m_referenceCounterPtr = aRef.m_referenceCounterPtr;
	m_data = aRef.m_data;
	(*m_referenceCounterPtr)++;

	return *this;
}

template<typename T>
inline Ref<T>& Ref<T>::operator=(T* aPtr)
{
	m_data = aPtr;
	m_referenceCounterPtr = &m_referenceCounter;
	m_referenceCounter = 0;
}

template<typename T>
inline bool Ref<T>::operator==(const Ref<T>& aRef) const
{
	return m_data == aRef.m_data;
}

template<typename T>
inline Ref<T>::operator bool() const
{
	return m_data != nullptr;
}

template<typename T>
inline bool Ref<T>::operator!() const
{
	return m_data == nullptr;
}

template<typename T>
inline void Ref<T>::Release()
{
	(*m_referenceCounterPtr)--;
	if ((*m_referenceCounterPtr) == 0)
	{
		delete m_data;
	}

	m_data = nullptr;
	m_referenceCounterPtr = nullptr;
}

template<typename T>
inline T* Ref<T>::operator->()
{
	return m_data;
}

template<typename T>
inline T& Ref<T>::operator*()
{
	return *m_data;
}

template<typename T>
inline T* Ref<T>::Get()
{
	return m_data;
}

template<typename T>
inline const size_t Ref<T>::GetRefCount() const
{
	return (*m_referenceCounterPtr);
}

template<typename T>
template<typename V>
inline Ref<V> Ref<T>::As()
{
 	Ref<V> newRef;
	
	newRef.m_data = reinterpret_cast<V*>(m_data);
	newRef.m_referenceCounterPtr = m_referenceCounterPtr;
	(*m_referenceCounterPtr)++;

	return newRef;
}

template<typename T, typename ... Args>
Ref<T> CreateRef(Args&&... args)
{
	return Ref<T>(new T(std::forward<Args>(args)...));
}