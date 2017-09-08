/*
 *  Copyright 2017 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package io.aeron;

import org.agrona.DirectBuffer;

/**
 * Vector into a {@link DirectBuffer} to be used for gathering IO as and offset and length.
 */
public final class DirectBufferVector
{
    public DirectBuffer buffer;
    public int offset;
    public int length;

    /**
     * Default constructor so the fluent API can be used.
     */
    public DirectBufferVector()
    {
    }

    /**
     * Construct a new vector as a subset of a buffer.
     *
     * @param buffer which is the super set.
     * @param offset at which the vector begins.
     * @param length of the vector.
     */
    public DirectBufferVector(final DirectBuffer buffer, final int offset, final int length)
    {
        this.buffer = buffer;
        this.offset = offset;
        this.length = length;
    }

    /**
     * Reset the values.
     *
     * @param buffer which is the super set.
     * @param offset at which the vector begins.
     * @param length of the vector.
     * @return this for a fluent API.
     */
    public DirectBufferVector reset(final DirectBuffer buffer, final int offset, final int length)
    {
        this.buffer = buffer;
        this.offset = offset;
        this.length = length;

        return this;
    }

    /**
     * The buffer which the vector applies to.
     *
     * @return buffer which the vector applies to.
     */
    public DirectBuffer buffer()
    {
        return buffer;
    }

    /**
     * The buffer which the vector applies to.
     *
     * @param buffer which the vector applies to.
     * @return this for a fluent API.
     */
    public DirectBufferVector buffer(final DirectBuffer buffer)
    {
        this.buffer = buffer;
        return this;
    }

    /**
     * Offset in the buffer at which the vector starts.
     *
     * @return offset in the buffer at which the vector starts.
     */
    public int offset()
    {
        return offset;
    }

    /**
     * Offset in the buffer at which the vector starts.
     *
     * @param offset in the buffer at which the vector starts.
     * @return this for a fluent API.
     */
    public DirectBufferVector offset(final int offset)
    {
        this.offset = offset;
        return this;
    }

    /**
     * Length of the vector in the buffer starting at the offset.
     *
     * @return length of the vector in the buffer starting at the offset.
     */
    public int length()
    {
        return length;
    }

    /**
     * Length of the vector in the buffer starting at the offset.
     *
     * @param length of the vector in the buffer starting at the offset.
     * @return this for a fluent API.
     */
    public DirectBufferVector length(final int length)
    {
        this.length = length;
        return this;
    }

    /**
     * Ensure the vector is valid for the buffer.
     *
     * @throws NullPointerException if the buffer is null.
     * @throws IllegalArgumentException if the offset is out of range for the buffer.
     * @throws IllegalArgumentException if the length is out of range for the buffer.
     * @return this for a fluent API.
     */
    public DirectBufferVector validate()
    {
        if (null == buffer)
        {
            throw new NullPointerException("buffer cannot be null");
        }

        final int capacity = buffer.capacity();
        if (offset < 0 || offset >= capacity)
        {
            throw new IllegalArgumentException("offset=" + offset + " capacity=" + capacity);
        }

        if (length < 0 || length > (capacity - offset))
        {
            throw new IllegalArgumentException("offset=" + offset + " capacity=" + capacity + " length=" + length);
        }

        return this;
    }
}
