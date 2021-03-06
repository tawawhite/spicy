.. rubric:: Methods

.. spicy:method:: time::nanoseconds time nanoseconds False real ()

    Returns the time as an integer value representing nanoseconds since
    the UNIX epoch.

.. spicy:method:: time::seconds time seconds False real ()

    Returns the time as a real value representing seconds since the UNIX
    epoch.

.. rubric:: Operators

.. spicy:operator:: time::Difference time t:time <sp> op:- <sp> t:interval

    Subtracts the interval from the time.

.. spicy:operator:: time::Difference time t:time <sp> op:- <sp> t:time

    Returns the difference of the times.

.. spicy:operator:: time::Equal bool t:time <sp> op:== <sp> t:time

    Compares two time values.

.. spicy:operator:: time::Greater bool t:time <sp> op:> <sp> t:time

    Compares the times.

.. spicy:operator:: time::GreaterEqual bool t:time <sp> op:>= <sp> t:time

    Compares the times.

.. spicy:operator:: time::Lower bool t:time <sp> op:< <sp> t:time

    Compares the times.

.. spicy:operator:: time::LowerEqual bool t:time <sp> op:<= <sp> t:time

    Compares the times.

.. spicy:operator:: time::Sum time t:time <sp> op:+ <sp> t:interval $commutative$

    Adds the interval to the time.

.. spicy:operator:: time::Unequal bool t:time <sp> op:!= <sp> t:time

    Compares two time values.

