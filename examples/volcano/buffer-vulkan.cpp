#include "buffer.h"
#include "vk-utils.h"

#include <stb_sprintf.h>

template <>
Buffer<Vk>::Buffer(Buffer&& other) noexcept
: DeviceObject(std::forward<Buffer>(other))
, myDesc(std::exchange(other.myDesc, {}))
, myBuffer(std::exchange(other.myBuffer, {}))
{
}

template <>
Buffer<Vk>::Buffer(
    const std::shared_ptr<DeviceContext<Vk>>& deviceContext,
    BufferCreateDesc<Vk>&& desc,
    ValueType&& buffer)
: DeviceObject(
    deviceContext,
    {"_Buffer"},
    1,
    VK_OBJECT_TYPE_BUFFER,
    reinterpret_cast<uint64_t*>(&std::get<0>(buffer)))
, myDesc(std::forward<BufferCreateDesc<Vk>>(desc))
, myBuffer(std::forward<ValueType>(buffer))
{
}

template <>
Buffer<Vk>::Buffer(
    const std::shared_ptr<DeviceContext<Vk>>& deviceContext,
    BufferCreateDesc<Vk>&& desc)
: Buffer(
    deviceContext,
    std::forward<BufferCreateDesc<Vk>>(desc),
    createBuffer(
        deviceContext->getAllocator(),
        desc.size,
        desc.usageFlags,
        desc.memoryFlags,
        "todo_insert_proper_name"))
{
}

template <>
Buffer<Vk>::Buffer(
    const std::shared_ptr<DeviceContext<Vk>>& deviceContext,
    CommandPoolContext<Vk>& commandContext,
    std::tuple<
        BufferCreateDesc<Vk>,
        BufferHandle<Vk>,
        AllocationHandle<Vk>>&& descAndInitialData)
: Buffer(
    deviceContext,
    std::forward<BufferCreateDesc<Vk>>(std::get<0>(descAndInitialData)),
    createBuffer(
        commandContext.commands(),
        deviceContext->getAllocator(),
        std::get<1>(descAndInitialData),
        std::get<0>(descAndInitialData).size,
        std::get<0>(descAndInitialData).usageFlags,
        std::get<0>(descAndInitialData).memoryFlags,
        "todo_insert_proper_name"))
{
    commandContext.addCommandsFinishedCallback([deviceContext, descAndInitialData](uint64_t){
        vmaDestroyBuffer(deviceContext->getAllocator(), std::get<1>(descAndInitialData), std::get<2>(descAndInitialData));
    });
}

template <>
Buffer<Vk>::Buffer(
    const std::shared_ptr<DeviceContext<Vk>>& deviceContext,
    CommandPoolContext<Vk>& commandContext,
    BufferCreateDesc<Vk>&& desc,
    const void* initialData)
: Buffer(
    deviceContext,
    commandContext, 
    std::tuple_cat(
        std::make_tuple(std::forward<BufferCreateDesc<Vk>>(desc)),
        createStagingBuffer(
            deviceContext->getAllocator(),
            initialData,
            desc.size,
            "todo_insert_proper_name")))
{
}

template <>
Buffer<Vk>::~Buffer()
{
    if (BufferHandle<Vk> buffer = *this; buffer)
        getDeviceContext()->addTimelineCallback(
            [allocator = getDeviceContext()->getAllocator(), buffer, bufferMemory = getBufferMemory()](uint64_t){
                vmaDestroyBuffer(allocator, buffer, bufferMemory);
        });
}

template <>
Buffer<Vk>& Buffer<Vk>::operator=(Buffer&& other) noexcept
{
    DeviceObject::operator=(std::forward<Buffer>(other));
    myDesc = std::exchange(other.myDesc, {});
    myBuffer = std::exchange(other.myBuffer, {});
    return *this;
}

template <>
void Buffer<Vk>::swap(Buffer& rhs) noexcept
{
    DeviceObject::swap(rhs);
    std::swap(myDesc, rhs.myDesc);
    std::swap(myBuffer, rhs.myBuffer);
}

template <>
BufferView<Vk>::BufferView(BufferView&& other) noexcept
: DeviceObject(std::forward<BufferView>(other))
, myView(std::exchange(other.myView, {}))
{
}

template <>
BufferView<Vk>::BufferView(
    const std::shared_ptr<DeviceContext<Vk>>& deviceContext,
    BufferViewHandle<Vk>&& view)
: DeviceObject(
    deviceContext,
    {"_View"},
    1,
    VK_OBJECT_TYPE_BUFFER_VIEW,
    reinterpret_cast<uint64_t*>(&view))
, myView(std::forward<BufferViewHandle<Vk>>(view))
{
}

template <>
BufferView<Vk>::BufferView(
    const std::shared_ptr<DeviceContext<Vk>>& deviceContext,
    const Buffer<Vk>& buffer,
    Format<Vk> format,
    DeviceSize<Vk> offset,
    DeviceSize<Vk> range)
: BufferView<Vk>(
    deviceContext,
    createBufferView(
        deviceContext->getDevice(),
        buffer,
        0, // "reserved for future use"
        format,
        offset,
        range))
{
}

template <>
BufferView<Vk>::~BufferView()
{
    if (BufferViewHandle<Vk> view = *this; view)
        getDeviceContext()->addTimelineCallback(
            [device = getDeviceContext()->getDevice(), view](uint64_t){
                vkDestroyBufferView(device, view, nullptr);
        });
}

template <>
BufferView<Vk>& BufferView<Vk>::operator=(BufferView&& other) noexcept
{
    DeviceObject::operator=(std::forward<BufferView>(other));
    myView = std::exchange(other.myView, {});
    return *this;
}

template <>
void BufferView<Vk>::swap(BufferView& rhs) noexcept
{
    DeviceObject::swap(rhs);
    std::swap(myView, rhs.myView);
}
